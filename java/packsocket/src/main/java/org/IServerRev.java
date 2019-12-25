package org;

public interface IServerRev
{
    public void Rev(byte mark, int cmd,byte[] recvBytes, int revSize);
}