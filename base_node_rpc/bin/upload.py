import sys

from arduino_rpc.upload import upload_firmware
from path_helpers import path
from .. import get_firmwares


def upload(board_name, port=None, arduino_install_home=None):
    firmware_path = get_firmwares()[board_name][0]
    upload_firmware(firmware_path, board_name, port, arduino_install_home)


def parse_args(args=None):
    """Parses arguments, returns (options, args)."""
    from argparse import ArgumentParser

    if args is None:
        args = sys.argv

    parser = ArgumentParser(description='Upload firmware to Arduino board.')
    parser.add_argument('board_name', type=path, default=None)
    parser.add_argument('-p', '--port', default=None)
    parser.add_argument('--arduino-install-home', type=path, default=None)

    args = parser.parse_args()
    return args


if __name__ == '__main__':
    args = parse_args()

    upload(args.board_name, args.port, args.arduino_install_home)
