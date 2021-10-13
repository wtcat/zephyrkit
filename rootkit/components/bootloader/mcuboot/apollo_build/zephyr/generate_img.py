#! /usr/bin/env python3
# coding:utf-8

import os
from zlib import crc32
import re
import time
import datetime

CONFIG_FILE_PATH = 'config/config.dat'

def find_info(file, key):
    with open(file, mode='r', encoding='utf-8') as f:
        for line in f.readlines():
            words = line.split()
            for i in words:
                n = re.findall(key, i)
                if n:
                    p1 = re.compile(r'["](.*?)["]', re.S)  # 最小匹配
                    res = re.findall(p1, line)
                    print(res)
                    return res[0]
    return 'null'
 
def convert_int_to_hex_string(num):
    a_bytes = bytearray()
    for i in range(0, 4):
        a_bytes.append((num >> ((3 - i) * 8)) & 0xff)
    aa = ''.join(['%02X' % b for b in a_bytes])
    return aa


def remove_files(ext_name):
    flist = os.listdir('.')
    for iter in flist:
        if os.path.splitext(iter)[1] == ext_name:
            print('Removed %s' % iter)
            os.remove(iter)
        

if __name__ == "__main__":
    app_bin_file = find_info(CONFIG_FILE_PATH, r"appBinFile")
    img_file = find_info(CONFIG_FILE_PATH, r"fullbin")
    boot_bin_file = find_info(CONFIG_FILE_PATH, r"bootloader")
    
    app_path = app_bin_file
    boot_file = boot_bin_file
    
    new_bin = bytearray()

    #Boot Image (Max: 64KB)
    with open(boot_file, 'rb') as f:
        file_data = f.read()
        for tmp in file_data:
            new_bin.append(tmp)

    for i in range(len(new_bin), 0x18000):
        new_bin.append(0xff)
    

    with open(app_path, 'rb') as f:
        file_data = f.read()
        for tmp in file_data:
            new_bin.append(tmp)
    
    file_crc32 = crc32(new_bin)
    print("crc32: %08X" % file_crc32)
    localTime = time.localtime(time.time())
    strTime = time.strftime("%Y%m%d%H%M%S", localTime)
    print(strTime)

 # Delete old binary file if exists
    #remove_files('.bin')

    crc_string = convert_int_to_hex_string(file_crc32)
    file_name = crc_string + '_' + img_file;
    
    with open(file_name, 'wb') as fw:
        fw.write(new_bin)
    
    with open(img_file, 'wb') as fw:
        fw.write(new_bin)
    
    print(file_name)
    print("file size: %d" % len(new_bin))
