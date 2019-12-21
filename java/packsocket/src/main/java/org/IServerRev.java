package org;

public interface IServerRev
{
    public void Rev(byte mark, byte[] recvBytes, int revSize, String erroMsg);
}