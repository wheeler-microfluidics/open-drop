from pprint import pprint
import sys

from paver.easy import task, needs, path, sh, cmdopts, options
from paver.setuputils import setup, find_package_data
import base_node_rpc

sys.path.insert(0, '.')
import version


PROJECT_PREFIX = path('PROJECT_PREFIX').bytes().strip()
package_files = find_package_data(package=PROJECT_PREFIX, where=PROJECT_PREFIX,
                                  only_in_packages=False)

DEFAULT_ARDUINO_BOARDS = ['uno', 'mega2560']
setup(name='wheeler.' + PROJECT_PREFIX,
      version=version.getVersion(),
      description='Arduino RPC node packaged as Python package.',
      author='Christian Fobel',
      author_email='christian@fobel.net',
      url='http://github.com/wheeler-microfluidics/%s.git' % PROJECT_PREFIX,
      license='GPLv2',
      install_requires=['arduino_scons', 'nadamq', 'path_helpers',
                        'arduino_helpers', 'wheeler.base_node_rpc>=0.3'],
      packages=[PROJECT_PREFIX],
      package_data=package_files)


@task
def generate_rpc_buffer_header():
    import arduino_rpc.rpc_data_frame as rpc_df

    output_dir = path(PROJECT_PREFIX).joinpath('Arduino', PROJECT_PREFIX)
    rpc_df.generate_rpc_buffer_header(output_dir)


@task
def generate_command_processor_header():
    from arduino_rpc.code_gen import write_code
    from arduino_rpc.rpc_data_frame import get_c_header_code

    sketch_dir = path(PROJECT_PREFIX).joinpath('Arduino', PROJECT_PREFIX)
    lib_dir = base_node_rpc.get_lib_directory()
    input_classes = ['BaseNode', 'Node']
    input_headers = [lib_dir.joinpath('BaseNode.h'),
                     sketch_dir.joinpath('Node.h')]

    output_header = path(PROJECT_PREFIX).joinpath('Arduino', PROJECT_PREFIX,
                                                  'NodeCommandProcessor.h')
    f_get_code = lambda *args_: get_c_header_code(*(args_ +
                                                    (PROJECT_PREFIX, )))

    write_code(input_headers, input_classes, output_header, f_get_code)


@task
def generate_python_code():
    from arduino_rpc.code_gen import write_code
    from arduino_rpc.rpc_data_frame import get_python_code

    sketch_dir = path(PROJECT_PREFIX).joinpath('Arduino', PROJECT_PREFIX)
    lib_dir = base_node_rpc.get_lib_directory()
    output_file = path(PROJECT_PREFIX).joinpath('node.py')
    input_classes = ['BaseNode', 'Node']
    input_headers = [lib_dir.joinpath('BaseNode.h'),
                     sketch_dir.joinpath('Node.h')]
    write_code(input_headers, input_classes, output_file, get_python_code)


@task
@needs('generate_command_processor_header', 'generate_rpc_buffer_header')
@cmdopts([('sconsflags=', 'f', 'Flags to pass to SCons.'),
          ('boards=', 'b', 'Comma-separated list of board names to compile '
           'for (e.g., `uno`).')])
def build_firmware():
    scons_flags = getattr(options, 'sconsflags', '')
    boards = [b.strip() for b in getattr(options, 'boards', '').split(',')
              if b.strip()]
    if not boards:
        boards = DEFAULT_ARDUINO_BOARDS
    for board in boards:
        # Compile firmware once for each specified board.
        sh('scons %s ARDUINO_BOARD="%s"' % (scons_flags, board))


@task
@needs('generate_setup', 'minilib', 'build_firmware', 'generate_python_code',
       'setuptools.command.sdist')
def sdist():
    """Overrides sdist to make sure that our setup.py is generated."""
    pass
