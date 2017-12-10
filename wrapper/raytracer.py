#!/usr/bin/python3

from pathlib import Path
import subprocess
import sys

def main():
    p = Path(__file__).parent
    if not Path(p, 'build/hw4').exists():
        print('The program must be built by running "make" first', file=sys.stderr)
        sys.exit(127)

    if len(sys.argv) == 2 and sys.argv[1] == '--yours':
        subprocess.run([
            Path(p, 'build/hw4'),
            '--no-preview',
            '--supersample',
            '4',
            '-s',
            '910,512',
            '-o',
            'yours.ppm',
            Path(p, '../scenes/chessboard.scn')
        ])
    elif len(sys.argv) == 2 and sys.argv[1] == '--default':
        subprocess.run([
            Path(p, 'build/hw4'),
            '--no-preview',
            '--supersample',
            '4',
            '-s',
            '512,512',
            '-o',
            'default.ppm',
            Path(p, '../scenes/cornell.scn')
        ])
    else:
        print('Usage: {} {{ --yours | --default }}'.format(sys.argv[0]), file=sys.stderr)

if __name__ == '__main__':
    main()
