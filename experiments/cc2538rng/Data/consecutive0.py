#!/usr/bin/python

import sys;

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

#Parse command line arguments.
def ParseCmdArgs(argc, argv):
    global datafile;

    if (argc < 2) or ('-h' in argv):
        print HelpMsg;
        exit();
    datafile = open(argv[1]);

    return;

def GetMaxConsecuiveZero(rndnum):
    nzero = list();
    count = 0;
    for i in range(0,len(rndnum)):
        if rndnum[i] == '0':
            count += 1;
        else:
            nzero.append(count);
            count = 0;
    nzero.append(count);

    return max(nzero);

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
    nzerolist = list();
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
        rndnum.replace('\0',"");
        rndnum.replace(' ',"");
        rndnum.replace('\r',"");
        rndnum.replace('\n','');
        l=list(rndnum);
        while '\n' in l:
            l.remove('\n');
        rndnum = ''.join(l);

        #print "{}({})".format(rndnum,len(rndnum)); #DEBUG info.
        
        #Interpret data as heximal.
        try:
            nbit = len(rndnum) * 4;
            rndval = int(rndnum,16);
            rndlist.append(rndval);
            rndbitstream = ('{:0'+str(nbit)+'b}').format(rndval);
        except:
            continue;

        nzerolist.append(GetMaxConsecuiveZero(rndbitstream));
        #print len(rndbitstream);

    npurezero = 0;
    for nzero in nzerolist:
        #print nzero;
        if nzero >= 64:
            npurezero += 1;
    print '#{} in {} ({:.4f}%)'.format(npurezero, len(nzerolist),\
            100 * float(npurezero) / len(nzerolist));
    
    print float(sum(nzerolist))/len(nzerolist);

    return;

if __name__ == "__main__":
    main(len(sys.argv),sys.argv);
