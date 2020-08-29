package org;

public class Param
{
    public  static final int REV_LEN = 1024;
    public  static final int MAX_LISTEN_NUM = 20;

    public  static final int GUID_LEN = 16;

    public static final int NewMsg_Size_LEN = 4;
    public static final int CMD_LEN = 4;

    public static final int Pos_cmd = 20;
    public static final int Pos_msg = 24;
    public static final int Pos_NewMsgMarkStrEnd = 1008;
    public static final int newMsgMaxDataSpace = 984;

    
    public  static final int MarkLen = 1;
    public  static final byte ConnMark_Def = (byte)0xFF;

    public static byte[] intToByte4(int i) { 
        byte[] targets = new byte[4]; 
        targets[0] = (byte) (i & 0xFF); 
        targets[1] = (byte) (i >> 8 & 0xFF); 
        targets[2] = (byte) (i >> 16 & 0xFF); 
        targets[3] = (byte) (i >> 24 & 0xFF); 
        return targets; 
      } 
    public  static int byte4ToInt(byte[] bytes, int off) { 
        int b3 = bytes[off] & 0xFF; 
        int b2 = bytes[off + 1] & 0xFF; 
        int b1 = bytes[off + 2] & 0xFF; 
        int b0 = bytes[off + 3] & 0xFF; 
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3; 
    } 
}