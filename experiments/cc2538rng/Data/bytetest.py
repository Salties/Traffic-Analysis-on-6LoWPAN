#!/usr/bin/python

import sys;

nbit = 16;
bitwidth = 0;

ContikiMsg= "\
Contiki-3.0\
TI SmartRF06 + cc2538EM\
 Net: sicslowpan\
 MAC: CSMA\
 RDC: ContikiMAC\
Rime configured with address 00:12:4b:00:04:1e:af:c6\
";

HelpMsg= "\
        Usage: bitest.py [HEX_FILE]\
        ";

bytecount = [0] * 256;

#Parse command line arguments.
def ParseCmdArgs(argc, argv):
    global datafile;

    if (argc < 2) or ('-h' in argv):
        print HelpMsg;
        exit();
    datafile = open(argv[1]);

    return;

#Main function
def main(argc, argv):
    global datafile;

    #Parse command line arguments.
    ParseCmdArgs(argc, argv);

    #Open log file.
    #datafile = open(argv[1]);
    rndbitstream = '';
 
    bitfile = -1;
    if argc > 2 :
        bitfile = open(argv[2],'w');

    #Read in data.
    rndnum = 1;
    rndlist = list();
    while rndnum:
        rndnum = datafile.readline();
        if rndnum == '\n':
            continue;
        if rndnum in ContikiMsg:
            continue;
        if rndnum == '':
            break;
        if rndnum[0] == '#':
            continue;
        #Remove illegal chracters.
        rndnum.replace('\0','');
        rndnum.replace(' ','');
        rndnum.replace('\n','');
        rndnumlen = len(rndnum);

        if rndnumlen > 4:
            rndnum = rndnum[:(rndnumlen - 3)];
        #print rndnum; #DEBUG info.

        #Interpret data as heximal.
        try:
            rndval = int(rndnum,16);
            rndlist.append(rndval);
            rndbitstream += ('{:0'+str(nbit)+'b}').format(rndval);
            bytecount[rndval] += 1;
        except:
            continue;

    #Total sample size.
    total = len(rndbitstream);
    print '#Total: {}'.format(total);
    nzero = rndbitstream.count('0');
    print '#{}/{} {:03f}%'.format(nzero, total - nzero, float(nzero)/float(total));

    totalbyte = total / 8;
    for i in range(0, len(bytecount)):
        print '{:02X}:\t{}/{}\t{:f}%'.format(i, bytecount[i], totalbyte, 100 * float(float(bytecount[i])/float(totalbyte)));

    #Store bit stream into a file if specified.
    if bitfile != -1:
        bitfile.write(rndbitstream);
    return;

if __name__ == "__main__":
    main(len(sys.argv),sys.argv);
