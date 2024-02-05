#include "msgloop.h"
int exitNum = 0;
extern int gProcNum;
extern int gPipeToMain[2];
void MonProcDataOption(MonProc *mpdata)
{
    if(mpdata->type == MPT_Exit)
        ++ exitNum;
}

void OutsideDataOption(Outside *odata)
{
    switch (odata->type) {
    case OT_AdmMon:
        sendManageInfo(&odata->info);
        break;
    case OT_AdmMain:
        break;
    default:
        break;
    }
}

int MainMessageLoop()
{
    MData data;
    while(1)
    {
        memset(&data,0,sizeof(data));
        read(gPipeToMain[0],&data,sizeof(data));
        switch (data.origin) {
        case MDO_MonProc:
            MonProcDataOption(&data.monproc);
            break;
        case MDO_Outside:
            OutsideDataOption(&data.outside);
            break;
        default:
            DMSG(ML_ERR,"Unknown MData Origin is %d\n",data.origin);
            break;
        }
        if(gProcNum == exitNum)
        {
            DMSG(ML_INFO,"Main process exit 0\n");
            break;
        }
    }
    return 0;
}
