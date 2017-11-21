#!/usr/bin/python -u

import sys;

windowsize = 128 * 100;
HelpMsg="Usage: Read stdin and computes frequency of bit 0."

def ParseCmdArg(argc, argv):
    if "-h" in argv:
        print HelpMsg;
        exit();

    return;

def main(argc, argv):
    #Parse command line argument.
    ParseCmdArg(argc, argv);

    windowbuffer = "";
    while 1:
        #Read in a newline
        newline = sys.stdin.readline();
        if newline == '':
            break;
        #Skip comments and blank line.
        if newline[0] in {'\n', '#'}:
            continue;
        #Compute the length of new bitstring. -1 for '\n'.
        rndlen = 4 * (len(newline) - 1);

        #Convert into binary form.
        try:
            rndval = int(newline, 16);
            bitstream = ('{:0' + str(rndlen) +'b}').format(rndval);
        except:
            continue;
        
        #Push data into the beginning of windowbuffer.
        windowbuffer = bitstream + windowbuffer;

        if len(windowbuffer) < windowsize: #Not enough data.
            print "#Waiting for more data...({}/{})".format(len(windowbuffer),windowsize);
            continue;
        else:   #Keep only the latest data.
            windowbuffer = windowbuffer[:windowsize];
        #Compute the bit frequency.
        nzero = windowbuffer.count('0');
        #print "{}/{}".format(nzero, windowsize);
        #print "{:.5f}%".format(float(nzero)/float(windowsize));
        print "{:.4f}".format(float(nzero)/float(windowsize));

    return;

if __name__ == "__main__":
    main(len(sys.argv),sys.argv);
