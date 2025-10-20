"""Install and build injected distributions"""

import os
import shutil
import subprocess
from setuptools import setup, find_packages
import sys


x86_cmake_arch_name, x64_cmake_arch_name = 'Win32', 'x64'
x86_package_arch_name, x64_package_arch_name = 'x86', 'x64'

is_64bit = sys.maxsize > 2**32
if is_64bit:
    arch_names_map = { x64_cmake_arch_name: x64_package_arch_name }
else:
    arch_names_map = { x86_cmake_arch_name: x86_package_arch_name }

build_dirname = 'build_'
build_dll_dirs = ['./backends/dotnet/', './backends/hook/', './backends/qt/']
package_dll_dirs = ['./src/injected/libs/dotnet/', './src/injected/libs/hook/', './src/injected/libs/qt/']
cmake_dirs = build_dll_dirs

# cmake build for DLLs
for arch in arch_names_map:
    for cmake_dir in cmake_dirs:
        build_dir = cmake_dir + build_dirname + arch

        os.makedirs(build_dir, exist_ok=True)

        subprocess.check_call(['cmake', '-B ' + build_dir, '-S ' + cmake_dir, '-A ' + arch])
        subprocess.check_call(['cmake', '--build', build_dir, '--config', 'Release'])

# copy DLLs to the package
for arch in arch_names_map:
    for build_dll_dir, package_dll_dir in zip(build_dll_dirs, package_dll_dirs):
        for root, _, files in os.walk(build_dll_dir + build_dirname + arch):
            for file in files:
                if file.endswith('.dll'):
                    src_file_path = os.path.join(root, file)
                    dest_file_path = os.path.join(package_dll_dir + arch_names_map.get(arch), file)
                    os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)
                    shutil.copy(src_file_path, dest_file_path)

setup(name='injected',
      version='0.0.1',
      description='A set of Python modules to inject DLls into applications for the Microsoft Windows',
      keywords="windows gui .net inject testing test desktop dll wpf",
      url="https://github.com/pywinauto/injected",
      author='Mark Mc Mahon and Contributors',
      author_email='pywinauto@yandex.ru',
      long_description="""
It allows to inject DLls into applications for the Microsoft Windows.
""",
      platforms=['win32'],

      packages=find_packages(where="src"),
      package_dir={"": "src"},
      include_package_data=True,

      license="BSD 3-clause",
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Console',
          'Intended Audience :: Developers',
          'License :: OSI Approved :: BSD License',
          'Operating System :: Microsoft :: Windows',
          'Programming Language :: Python',
          'Programming Language :: Python :: 3.7',
          'Programming Language :: Python :: 3.8',
          'Programming Language :: Python :: 3.9',
          'Programming Language :: Python :: 3.10',
          'Programming Language :: Python :: 3.11',
          'Programming Language :: Python :: Implementation :: CPython',
          'Topic :: Software Development :: Libraries :: Python Modules',
          'Topic :: Software Development :: Testing',
          'Topic :: Software Development :: User Interfaces'
      ],
      install_requires=['pywin32'],
      python_requires='>=3.7',
      )
