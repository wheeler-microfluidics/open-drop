import distutils.sysconfig
import re
import sys
from importlib import import_module

from path_helpers import path
import arduino_scons.auto_config
from arduino_scons.git_util import GitUtil

project_name = [d for d in path('.').dirs()
                if d.joinpath('Arduino').isdir()
                and d.name not in ('build', )][0].name
rpc_module = import_module(project_name)


def get_version_string():
    version = GitUtil(None).describe()
    branch = GitUtil(None).branch()
    if branch == "master":
        tags = ""
    else:
        tags = "-" + branch
    m = re.search('^v(?P<major>\d+)\.(?P<minor>\d+)(-(?P<micro>\d+))?', version)
    if m.group('micro'):
        micro = m.group('micro')
    else:
        micro = '0'
    return "%s.%s.%s%s" % (m.group('major'), m.group('minor'), micro, tags)


PYTHON_VERSION = "%s.%s" % (sys.version_info[0],
                            sys.version_info[1])

env = Environment()

SOFTWARE_VERSION = get_version_string()
Export('SOFTWARE_VERSION')

Import('PYTHON_LIB')

# # Build Arduino binaries #
sketch_build_root = path('build/arduino').abspath()
Export('sketch_build_root')
SConscript(rpc_module.get_sketch_directory().joinpath('SConscript'))

Import('arduino_hex')
Import('build_context')

# # Install compiled firmwares to `firmware` directory #
firmware_path = rpc_module.package_path().joinpath('firmware',
                                                   build_context.ARDUINO_BOARD)
package_hex = env.Install(firmware_path, arduino_hex)
