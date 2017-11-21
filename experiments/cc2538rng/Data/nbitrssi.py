#!/usr/bin/python

import sys;

nmax = 8000;

def main(argc, argv):
    datfile = open(argv[1]);
    datline = 1;
    while datline:
        datline = datfile.readline();
        if not datline:
            break;

        datfields = datline.split();
        if datfields[0] == '#':
            nbit = datfields[1];
            rssi = datfields[2];
            if int(nbit) == 8000:
                continue;
            print '{} {}'.format(nbit, rssi);

    return;

if __name__ == "__main__":
    main(len(sys.argv),sys.argv);
