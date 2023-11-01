# author: wangye(Wayne)
# license: Apache Licence
# file: generate_name_to_path_map.py
# time: 2023-11-02-02:06:44
# contact: wang121ye@hotmail.com
# site:  wangyendt@github.com
# software: PyCharm
# code is far away from bugs.


import os
import yaml
import re


def find_leaf_directories(path: str, outliers: set):
    """
    Find all leaf directories under the given path.

    Parameters:
    - path: the root directory to start the search from
    - outliers: a set of directories to be excluded from the search

    Returns:
    - A dict of leaf directories
    """
    leaf_directories = dict()
    for root, dirs, files in os.walk(path):
        if not dirs and not any(key in root for key in outliers):
            path = re.findall('^[^/]*/(.*)$', root)[0]
            assert os.path.basename(root) not in leaf_directories
            leaf_directories[os.path.basename(root)] = path
    return leaf_directories


yaml_path = 'name_to_path_map.yaml'
kvp = find_leaf_directories('.', {'.git', '.idea'})
with open(yaml_path, 'w') as f:
    yaml.dump(kvp, f)
