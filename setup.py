"""Install and build injectlib distributions"""

import os
import shutil
import subprocess
import sys
from setuptools import setup
from setuptools.command.sdist import sdist
from setuptools.dist import Distribution


x86_cmake_arch_name, x64_cmake_arch_name = 'Win32', 'x64'
x86_package_arch_name, x64_package_arch_name = 'x86', 'x64'

is_64bit = sys.maxsize > 2**32
if is_64bit:
    arch_names_map = { x64_cmake_arch_name: x64_package_arch_name }
else:
    arch_names_map = { x86_cmake_arch_name: x86_package_arch_name }

build_dirname = 'build_'
build_dll_dirs = ['./backends/dotnet/',
                  './backends/hook/',
                  './backends/qt5/']
package_dll_dirs = ['./src/injectlib/libs/dotnet/',
                    './src/injectlib/libs/hook/',
                    './src/injectlib/libs/qt5/']
if is_64bit:
    build_dll_dirs.append('./backends/qt6/')
    package_dll_dirs.append('./src/injectlib/libs/qt6/')
cmake_dirs = build_dll_dirs


class SdistWithDlls(sdist):
    """Custom build command that compiles DLLs before packaging Python files."""

    def run(self):
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
        super().run()

# mark the package is not pure Python code
class BinaryDistribution(Distribution):
    def is_pure(self):
        return False


setup(name='injectlib',
      version='0.0.3',
      description='A set of Python modules to inject DLLs into applications for the Microsoft Windows',
      keywords="windows gui .net inject testing test desktop dll wpf qt",
      url="https://github.com/pywinauto/injectlib",
      project_urls={
          "GitHub": "https://github.com/pywinauto/injectlib",
      },
      author='Vasily Ryabov, Ilya Naumov, Boris Galochkin, Alexander Makarov and contributors',
      author_email='pywinauto-users@lists.sourceforge.net',
      long_description="""
injectlib is a Python package that allows you to inject DLLs into running
processes on Windows (32 or 64-bit).

This can be useful for various tasks, such as debugging or adding
functionality to existing applications.

This is initially created for desktop GUI automation purpose
(project [pywinauto](https://github.com/pywinauto/pywinauto)), because
an injected DLL can access GUI widgets' text properties with much better
precision and coverage than standard OS APIs like MS UI Automation API or
Win32 API. Other use cases are potentially possible at
your own risk according to the law and the project license.
""",
      platforms=['win32'],

      packages=["injectlib"],
      package_dir={"": "src"},
      include_package_data=True,

      license="BSD-3-Clause",
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Console',
          'Intended Audience :: Developers',
          'License :: OSI Approved :: BSD License',
          'Operating System :: Microsoft :: Windows',
          'Programming Language :: Python',
          'Programming Language :: Python :: 3',
          'Programming Language :: Python :: 3 :: Only',
          'Programming Language :: Python :: 3.7',
          'Programming Language :: Python :: 3.8',
          'Programming Language :: Python :: 3.9',
          'Programming Language :: Python :: 3.10',
          'Programming Language :: Python :: 3.11',
          'Programming Language :: Python :: 3.12',
          'Programming Language :: Python :: Implementation :: CPython',
          'Topic :: Software Development :: Libraries :: Python Modules',
          'Topic :: Software Development :: Testing',
      ],
      install_requires=['pywin32'],
      python_requires='>=3.7',
      cmdclass={'sdist': SdistWithDlls},
      )
