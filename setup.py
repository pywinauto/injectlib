"""Install and build injected distributions"""

from setuptools import setup, find_packages

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
