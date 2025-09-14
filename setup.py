# Copyright (C) 2023 Mark Mc Mahon and Contributors
# https://github.com/pywinauto/injected/graphs/contributors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of pywinauto nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Install and build injectdll distributions"""

import os
import shutil
import subprocess
from setuptools import setup, find_packages
from setuptools.dist import Distribution
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
package_dll_dirs = ['./src/injectdll/libs/dotnet/', './src/injectdll/libs/hook/', './src/injectdll/libs/qt/']
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


# mark the package is not pure Python code
class BinaryDistribution(Distribution):
    def is_pure(self):
        return False


setup(name='injectdll',
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
      install_requires=['six', 'pywin32'],
      python_requires='>=3.7',
      )
