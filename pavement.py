from collections import OrderedDict
import sys
from importlib import import_module

from paver.easy import task, needs, path, sh, cmdopts, options
from paver.setuputils import setup, find_package_data, install_distutils_tasks
try:
    from base_node_rpc.pavement_base import *
except ImportError:
    pass

sys.path.insert(0, '.')
import version
install_distutils_tasks()

DEFAULT_ARDUINO_BOARDS = ['uno', 'mega2560']
PROJECT_PREFIX = [d for d in path('.').dirs()
                  if d.joinpath('Arduino').isdir()][0].name
rpc_module = import_module(PROJECT_PREFIX)
VERSION = version.getVersion()
URL='http://github.com/wheeler-microfluidics/%s.git' % PROJECT_PREFIX
PROPERTIES = OrderedDict([('name', PROJECT_PREFIX),
                          ('manufacturer', 'Wheeler Lab'),
                          ('software_version', VERSION),
                          ('url', URL)])
package_files = find_package_data(package=PROJECT_PREFIX, where=PROJECT_PREFIX,
                                  only_in_packages=False)

options(
    rpc_module=rpc_module,
    PROPERTIES=PROPERTIES,
    base_classes=['BaseNodeI2cHandler'],
    rpc_classes=['demo_rpc::Node'],
    DEFAULT_ARDUINO_BOARDS=DEFAULT_ARDUINO_BOARDS,
    setup=dict(name='wheeler.' + PROJECT_PREFIX,
               version=VERSION,
               description='Arduino RPC node packaged as Python package.',
               author='Christian Fobel',
               author_email='christian@fobel.net',
               url=URL,
               license='GPLv2',
               install_requires=['wheeler.base_node_rpc>=0.8'],
               packages=[PROJECT_PREFIX],
               package_data=package_files))
