#!/usr/bin/python

#Fixed zephyr cmake generate error with no userspace
#Sync zephyr west submodule config

import os

# def delete_folder(path):
#   folder_name_exist = os.path.exists(path)
#   if not folder_name_exist:
#     return
#   for i in os.listdir(path):
#       path_file = os.path.join(path, i)
#       if os.path.isfile(path_file):
#           os.remove(path_file)
#       else:
#           delete_folder(path_file)
#   os.removedirs(path)

# def create_folder(path):
#     result_data = 0
#     folder_name_exist = os.path.exists(path)
#     if not folder_name_exist:
#         os.makedirs(path)
#     if not os.path.exists(path):
#         result_data = 1
#     return result_data

def add_patch(path):
  with open(path, "r+", encoding="utf-8") as fd:
    fbuffer = fd.read()
    if fbuffer.find("zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)") < 0:
      print("Writing:", path)
      fd.seek(0, 2)
      fd.write("\nzephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)")
    
def fixed_cmake_file(root_path, *k):
  path_list = []
  for dname in k:
    abs_path = os.path.join(root_path, "drivers/", dname, "CMakeLists.txt")
    add_patch(abs_path)

if __name__ == "__main__":
  fixed_cmake_file(os.path.join(os.getcwd(), 'zephyr'), 
    'gpio', 'flash', 'spi', 'serial', 'kscan', 'pwm', 'disk')

