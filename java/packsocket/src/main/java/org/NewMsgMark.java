package org;

class newMsgMark
{
    public newMsgMark()
    {
        String _NewMsgMarkStr = "6DB62435625348A28426C876F6B04A88";
        NewMsgMarkStr = _NewMsgMarkStr.getBytes();

        String _NewMsgMarkStrEnd = "CE1D1E7C9DBF4951BB414B6D9D2AA1C1";
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