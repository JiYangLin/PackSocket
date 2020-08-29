package org;

class newMsgMark
{
    public newMsgMark()
    {
        String _NewMsgMarkStr = "4A6959616E674C696E6DB62435625348";
        NewMsgMarkStr = _NewMsgMarkStr.getBytes();

        String _NewMsgMarkStrEnd = "414B6D9D2AA1C14A6959616E674C696E";
        NewMsgMarkStrEnd = _NewMsgMarkStrEnd.getBytes();
    }
    public byte[] NewMsgMarkStr;
    public byte[] NewMsgMarkStrEnd;
    public boolean isNewMsgMark(byte[] recvBuf)
    {
        int guidLen = (int)Param.GUID_LEN;
        for (int i = 0; i < guidLen; ++i)
            if (NewMsgMarkStr[i] != recvBuf[i]) return false;


        int EndGDIDPos = (int)Param.REV_LEN - guidLen;
        for (int i = 0; i < guidLen; ++i)
            if (NewMsgMarkStrEnd[i] != recvBuf[i + EndGDIDPos]) return false;

        return true;
    }
}