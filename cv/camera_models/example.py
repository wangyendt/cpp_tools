# author: Wayne
# license: Apache Licence
# file: example.py
# time: 2024-10-10
# contact: wang121ye@hotmail.com
# site:  wangyendt@github.com
# software: PyCharm
# code is far away from bugs.

import sys
import os
import numpy as np

# --- Setup --- 
sys.path.append('./lib')
os.system("mkdir -p build && cd build && cmake .. && make -j")


# Attempt to import the module
try:
    import camera_models
    # Import the YAML helper functions
    from pywayne.cv.tools import read_cv_yaml, write_cv_yaml
except ImportError as e:
    print(f"Error importing modules: {e}")
    print(f"Please ensure camera_models is built and pywayne is installed.")
    print("Build command: mkdir -p build && cd build && cmake .. && make -j")
    sys.exit(1)

# Define the sample YAML file path
sample_yaml_path = os.path.join('.', 'pinhole_test.yaml')
modified_yaml_path = os.path.join('.', 'pinhole_modified.yaml')

# --- Helper to create sample YAML using write_cv_yaml ---
def create_sample_yaml_if_needed(file_path):
    if not os.path.exists(file_path):
        print(f"Creating sample config using write_cv_yaml: {file_path}")
        sample_data = {
            "model_type": "PINHOLE",
            "camera_name": "test_pinhole",
            "image_width": 640,
            "image_height": 480,
            "distortion_parameters": {
                "k1": -0.28340811,
                "k2": 0.07395907,
                "p1": 0.00019359,
                "p2": 1.76187114e-05
            },
            "projection_parameters": {
                "fx": 458.654,
                "fy": 457.296,
                "cx": 367.215,
                "cy": 248.375
            }
        }
        if not write_cv_yaml(file_path, sample_data):
             print(f"Error: Failed to write sample YAML to {file_path}")
             sys.exit(1)
        else:
             print(f"Successfully wrote sample YAML.")

# --- Main Example --- 
def run_example():
    print("--- Camera Models Pybind Example ---")

    # Ensure sample YAML exists
    create_sample_yaml_if_needed(sample_yaml_path)

    # 1. Get CameraFactory instance
    print("\n1. Getting CameraFactory instance...")
    factory = camera_models.CameraFactory.instance()
    print(f"  Factory instance: {factory}")

    # 2. Generate camera from YAML file (using C++ loader)
    print(f"\n2. Loading camera directly from YAML: {sample_yaml_path}...")
    try:
        pinhole_cam_from_yaml = factory.generate_camera_from_yaml_file(sample_yaml_path)
        if pinhole_cam_from_yaml:
            print("  Successfully loaded PinholeCamera directly from YAML.")
            print(f"  Camera Name: {pinhole_cam_from_yaml.camera_name}")
            print(f"  Model Type: {pinhole_cam_from_yaml.model_type}")
            # print(f"  Parameters:\n{pinhole_cam_from_yaml}") # Uses __repr__
        else:
            print(f"  Error: Could not load camera from {sample_yaml_path}")
            return
    except Exception as e:
        print(f"  Error loading camera from YAML: {e}")
        return

    # Optional: Demonstrate reading with Python reader
    # print(f"\n2b. Reading YAML with Python reader: {sample_yaml_path}...")
    # data_read = read_cv_yaml(sample_yaml_path)
    # if data_read:
    #      print("  Successfully read YAML with Python reader.")
    #      # print(data_read)
    # else:
    #      print("  Failed to read YAML with Python reader.")

    # 3. Generate camera by type
    print("\n3. Generating a new camera by type...")
    img_width = 1280
    img_height = 720
    cam_name = "my_pinhole"
    pinhole_cam_new = factory.generate_camera(camera_models.ModelType.PINHOLE, cam_name, (img_width, img_height))
    print(f"  Generated camera: {pinhole_cam_new.camera_name}, Type: {pinhole_cam_new.model_type}")
    print(f"  Image Size: {pinhole_cam_new.image_width}x{pinhole_cam_new.image_height}")

    # 4. Access and modify parameters
    print("\n4. Accessing and modifying parameters...")
    params = pinhole_cam_new.get_parameters() # Get a copy
    print(f"  Initial fx: {params.fx}")
    params.fx = 600.0
    params.fy = 600.0
    params.cx = img_width / 2.0 - 0.5
    params.cy = img_height / 2.0 - 0.5
    params.k1 = 0.01 # Add some distortion
    print(f"  Modified fx: {params.fx}, k1: {params.k1}")

    # Set parameters back to the camera object
    pinhole_cam_new.set_parameters(params)
    print("  Updated parameters set.")
    updated_params = pinhole_cam_new.get_parameters()
    print(f"  Camera's new fx: {updated_params.fx}")
    print(f"  Camera parameters:\n{pinhole_cam_new}")

    # 5. Use projection and lifting functions
    print("\n5. Testing projection and lifting...")
    # Use the camera loaded from YAML as it has distortion
    test_cam = pinhole_cam_from_yaml
    print(f"  Using camera: {test_cam.camera_name}")

    # Define a 3D point (in camera coordinates)
    point_3d = np.array([0.5, -0.2, 2.0]) # x, y, z
    print(f"  3D point (camera frame): {point_3d}")

    # Project to 2D image plane
    point_2d = test_cam.space_to_plane(point_3d)
    print(f"  Projected 2D point (distorted): {point_2d}")

    # Lift the 2D point back to a 3D ray
    lifted_ray = test_cam.lift_projective(point_2d)
    print(f"  Lifted 3D ray (before normalization): {lifted_ray}")

    # Normalize the ray
    normalized_ray = lifted_ray * (point_3d[2] / lifted_ray[2])
    print(f"  Lifted 3D ray (normalized to original Z): {normalized_ray}")

    # Check reprojection error (should be small)
    reprojection_error = np.linalg.norm(point_3d - normalized_ray)
    print(f"  Reprojection Error (3D): {reprojection_error:.6f}")

    # Test undistToPlane
    p_u_lifted = np.array([lifted_ray[0]/lifted_ray[2], lifted_ray[1]/lifted_ray[2]])
    print(f"  Corresponding undistorted normalized point: {p_u_lifted}")
    reprojected_2d = test_cam.undist_to_plane(p_u_lifted)
    print(f"  Reprojected 2D point using undistToPlane: {reprojected_2d}")
    reprojection_error_2d = np.linalg.norm(point_2d - reprojected_2d)
    print(f"  Reprojection Error (2D): {reprojection_error_2d:.6f}")

    # 6. Test another camera type (e.g., Kannala-Brandt / Equidistant)
    print("\n6. Generating Equidistant camera...")
    equi_cam = factory.generate_camera(camera_models.ModelType.KANNALA_BRANDT, "fisheye", (800, 600))
    equi_params = equi_cam.get_parameters()
    equi_params.k2 = 0.01
    equi_params.k3 = 0.005
    equi_params.k4 = -0.001
    equi_params.k5 = 0.0
    equi_params.mu = 400.0
    equi_params.mv = 400.0
    equi_params.u0 = 800 / 2.0
    equi_params.v0 = 600 / 2.0
    equi_cam.set_parameters(equi_params)
    print(f"  Equidistant camera parameters:\n{equi_cam}")
    point_3d_fisheye = np.array([0.8, 0.1, 1.5])
    point_2d_fisheye = equi_cam.space_to_plane(point_3d_fisheye)
    print(f"  3D point: {point_3d_fisheye} -> 2D point: {point_2d_fisheye}")
    lifted_ray_fisheye = equi_cam.lift_projective(point_2d_fisheye)
    normalized_ray_fisheye = lifted_ray_fisheye * (point_3d_fisheye[2] / lifted_ray_fisheye[2])
    print(f"  Lifted ray (normalized): {normalized_ray_fisheye}")
    print(f"  Reprojection Error (3D): {np.linalg.norm(point_3d_fisheye - normalized_ray_fisheye):.6f}")

    # 7. Write modified parameters to a new YAML file
    print(f"\n7. Writing modified Pinhole camera parameters to {modified_yaml_path}...")
    # Get the parameters from the modified camera
    modified_params_obj = pinhole_cam_new.get_parameters()
    # Create a dictionary structure suitable for write_cv_yaml
    data_to_write = {
        "model_type": "PINHOLE", # Need to manually add model type
        "camera_name": modified_params_obj.camera_name,
        "image_width": modified_params_obj.image_width,
        "image_height": modified_params_obj.image_height,
        "distortion_parameters": {
            "k1": modified_params_obj.k1,
            "k2": modified_params_obj.k2,
            "p1": modified_params_obj.p1,
            "p2": modified_params_obj.p2
        },
        "projection_parameters": {
            "fx": modified_params_obj.fx,
            "fy": modified_params_obj.fy,
            "cx": modified_params_obj.cx,
            "cy": modified_params_obj.cy
        }
    }
    if write_cv_yaml(modified_yaml_path, data_to_write):
        print(f"  Successfully wrote modified parameters to {modified_yaml_path}")
    else:
        print(f"  Error writing modified parameters to {modified_yaml_path}")


    print("\n--- Example Finished ---")

if __name__ == "__main__":
    run_example() 