/*======================================================== 
**University of Illinois/NCSA 
**Open Source License 
**
**Copyright (C) 2007-2008,The Board of Trustees of the University of 
**Illinois. All rights reserved. 
**
**Developed by: 
**
**    Research Group of Professor Sam King in the Department of Computer 
**    Science The University of Illinois at Urbana-Champaign 
**    http://www.cs.uiuc.edu/homes/kingst/Research.html 
**
**Permission is hereby granted, free of charge, to any person obtaining a 
**copy of this software and associated documentation files (the 
**"Software"), to deal with the Software without restriction, including 
**without limitation the rights to use, copy, modify, merge, publish, 
**distribute, sublicense, and/or sell copies of the Software, and to 
**permit persons to whom the Software is furnished to do so, subject to 
**the following conditions: 
**
*** Redistributions of source code must retain the above copyright notice, 
**this list of conditions and the following disclaimers. 
*** Redistributions in binary form must reproduce the above copyright 
**notice, this list of conditions and the following disclaimers in the 
**documentation and/or other materials provided with the distribution. 
*** Neither the names of <Name of Development Group, Name of Institution>, 
**nor the names of its contributors may be used to endorse or promote 
**products derived from this Software without specific prior written 
**permission. 
**
**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
**EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
**MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
**IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
**ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
**TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
**SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE. 
**========================================================== 
*/
package message;

import java.io.*;
import java.util.concurrent.locks.ReentrantLock;
import java.awt.Rectangle;
import java.util.StringTokenizer;
import java.awt.Dimension;

// XXX FIXME  there are syncronization issues here.  We need to make sure
// that only one thread can read or write at any time, we need to protect
// our static structures (e.g., urlID), and we need to implement a message
// queue to cope with out of order messages recieved when waiting on
// a syncronous call

public class Message {
    public static final int INT_SIZE = 4;

    public static final int KERNEL_ID = 0;
    public static final int WEBAPPMGR_ID = 1;
    public static final int CACHE_ID = 2;
    public static final int UI_ID = 3;
    public static final int STORAGE_ID = 4;
    public static final int COOKIE_ID = STORAGE_ID;
    public static final int VNC_SERVER_ID = 5;
    public static final int AUDITLOG_ID = 6;
    public static final int EXTENSION_ID = 7;
    public static final int JS_ID = 8;
    public static final int PLUGIN_ID = 9;

    public static final int WEBAPP_FIRST_ID = 1024;

    public static final int MSG_NEW_URL = 0;
    public static final int MSG_FETCH_URL = 1;
    public static final int MSG_RETURN_URL = 2;
    public static final int MSG_SET_URL = 3;
    public static final int MSG_UPDATE_URL = 4;
    public static final int MSG_REQUEST_PIXELS = 5;
    public static final int MSG_UPDATE_PIXELS = 6;
    public static final int MSG_UPDATE_WINDOW_SIZE = 7;
    public static final int MSG_SET_LOCATION_BAR = 8;
    public static final int MSG_SET_STATUS_BAR = 9;
    public static final int MSG_SET_CAPTION = 10;
    public static final int MSG_FROM_VNC_CLIENT = 11;
    public static final int MSG_FROM_VNC_SERVER = 12;
    public static final int MSG_VNC_INIT = 13;
    /*
    public static final int MSG_NAV_BACK = 14;
    public static final int MSG_NAV_FWD = 15;
    public static final int MSG_NAV_REFRESH = 16;
    */
    public static final int MSG_NAV_SET_WEBAPP = 14;
    public static final int MSG_NEW_WEBAPP = 15;
    public static final int MSG_NAV_STOP = 17;
    public static final int MSG_GET_OBJECT_AUDIT_RECORDS = 18;
    public static final int MSG_QUERY_AUDIT_LOG_REPLY = 19;
    public static final int MSG_EXTENSION_ACTION = 20;
    public static final int MSG_STORE_OBJECT = 21;
    public static final int MSG_RETRV_OBJECT = 22;
    public static final int MSG_RETURN_URL_METADATA = 23;
    public static final int MSG_OBJECT_ADD_ACL_USER = 24;
    public static final int MSG_OBJECT_REM_ACL_USER = 25;
    public static final int MSG_OBJECT_DOWNLOAD_READY = 26;
    public static final int MSG_JS_EVALUATE = 27;
    public static final int MSG_JS_REPLY = 28;
    public static final int MSG_JS_SET_HANDLER_CODE = 29;
    public static final int MSG_JS_INVOKE_HANDLER = 30;
    public static final int MSG_SNAP_SHOT = 31;
    public static final int MSG_REPLAY_WEBAPP = 32;
    public static final int MSG_SET_POLICY           =  33 ;
    public static final int MSG_LOG_BEHAVIOR_DATA    =  34 ;
    public static final int MSG_LOG_USER_INPUT       =  35 ;
    public static final int MSG_SET_REPLAY_MODE      =  36 ;
    public static final int MSG_SAVE_DOM             =  37 ;


    public static final int MSG_COOKIE_SET =            1007;
    public static final int MSG_COOKIE_REMOVE =         1008;
    public static final int MSG_COOKIE_LISTDOMAINS =    1009;
    public static final int MSG_COOKIE_GET =            1010;
    public static final int MSG_COOKIE_NEWJAR =         1011;
    public static final int MSG_COOKIE_NUMJARS =        1012;
    public static final int MSG_COOKIE_USEJAR =         1013;
    
    public static final int MSG_PLUGIN_NPN =            1020;
    public static final int MSG_PLUGIN_NPP =            1021;

    public static final int MSG_DOM_COOKIE_GET =         1030;
    public static final int MSG_DOM_COOKIE_SET =         1031;

    public static final int MSG_WEBAPP_MSG =             4000;
    public static final int MSG_WEBAPP_CLOSE =           4000;

    public static final int MSG_UI_MSG =                 5000; 

    public static final int MSG_loadStarted =            5000;
    public static final int MSG_loadProgress =           5001;
    public static final int MSG_loadFinished =           5002;
    public static final int MSG_linkHovered =            5003;
    public static final int MSG_statusBarMessage =       5004;
    public static final int MSG_geometryChangeRequested = 5005;
    public static final int MSG_windowCloseRequested = 5006;
    public static final int MSG_toolBarVisibilityChangeRequested = 5007;
    public static final int MSG_statusBarVisibilityChangeRequested = 5008;
    public static final int MSG_menuBarVisibilityChangeRequested = 5009;
    public static final int MSG_titleChanged =           5010;
    public static final int MSG_iconChanged =            5011;
    public static final int MSG_urlChanged =             5012;
    public static final int MSG_addHistoryItem =         5013;
    public static final int MSG_navBackOrForward =       5014;
    public static final int MSG_webAppExited =           5015;

    // these are for the request pixels and update pixels messages
    public static final int PIX_X_OFFSET = 0;
    public static final int PIX_Y_OFFSET = 1;
    public static final int PIX_W_OFFSET = 2;
    public static final int PIX_H_OFFSET = 3;

    private static final DataInputStream in = new DataInputStream(System.in);
    private static final DataOutputStream out = new DataOutputStream(System.out);
    private static ReentrantLock writeLock = new ReentrantLock();

    private MessageHeader header;
    private byte[] msgData;

    public Message() {
        header = new MessageHeader();
        msgData = null;
    } 

    public Message(int dstId, int msgType, int msgValue, String data) {
        header = new MessageHeader();
        setMsgData(dstId, msgType, msgValue, data);
    }

    public String toString() {
        return "srcId = " +header.srcId+ " dstId = " +header.dstId+ " msgType = " +header.msgType + 
            " msgValue = " + header.msgValue + " msgId = " + header.msgId + " dataLen = " + header.dataLen;
    }

    /************************** kernel calls **********************/
    public void newUrl(String url) {
        setMsgData(KERNEL_ID, MSG_NEW_URL, 1, url);
        writeMessage();
    }

    public void replayWebApp(int webAppId, String policies) {
        setMsgData(KERNEL_ID, MSG_REPLAY_WEBAPP, webAppId, policies);
        writeMessage();
    }

    public void newPostUrl(String url, String data) {
        // java should not use this, only the webapp
        assert(false);
        //newUrl("POST:" + url + '\0' + data);
    }

    /**************************************************************/

    /************************* WebApp calls ***********************/
    public void kjsEvaluate(int webAppId, int ctxId, String data) {
        setMsgData(webAppId, MSG_JS_EVALUATE, ctxId, data);
        writeMessage();
    }
    public void kjsReply(int webAppId, int ctxId, String data) {
        setMsgData(webAppId, MSG_JS_REPLY, ctxId, data);
        writeMessage();
    }
    public String getJsCode() {
        assert(header.msgType == MSG_JS_EVALUATE);
        return (new String(msgData)).substring(getColonIdx(1)+1);
    }

    public String getJsDomain() {
        assert((header.msgType == MSG_JS_EVALUATE) ||
               (header.msgType == MSG_JS_SET_HANDLER_CODE) ||
               (header.msgType == MSG_JS_INVOKE_HANDLER));

        return (new String(msgData)).substring(0, getColonIdx(1));
    }
    public Integer getJsHandlerId() {
        int ret = 0;
        try {
            if(header.msgType == MSG_JS_SET_HANDLER_CODE) {
                ret = Integer.parseInt((new String(msgData)).substring(getColonIdx(1)+1, getColonIdx(2)));
            } else if(header.msgType == MSG_JS_INVOKE_HANDLER) {
                ret = Integer.parseInt((new String(msgData)).substring(getColonIdx(1)+1));
            } else {
                assert(false);
            }
        } catch (Exception e) {
            e.printStackTrace();
            System.err.println("msgData = " + new String(msgData));
            assert(false);
        }
        return new Integer(ret);
    }
    public String getJsHandlerCode() {
        String code = new String();

        if(header.msgType == MSG_JS_SET_HANDLER_CODE) {
            code = (new String(msgData)).substring(getColonIdx(2)+1);
        } else {
            assert(false);
        }

        return code;
    }
    /*************************************************************/


    /************************ cache manager calls *****************/
    public void fetchUrl(int urlId, String url) {
        setMsgData(CACHE_ID, MSG_FETCH_URL, urlId, url);
        writeMessage();
    }
    /*************************************************************/


    /**********************  Web App Manager Calls ***************/
    public void navSetWebApp(int webAppId) {
        setMsgData(WEBAPPMGR_ID, MSG_NAV_SET_WEBAPP, webAppId);
        writeMessage();
    }

    public void navStop() {
        setMsgData(WEBAPPMGR_ID, MSG_NAV_STOP, 0);
        writeMessage();
    }

    /*************************************************************/

    /******************** VNC Server calls **************************/
    public void vncClientMsg(byte [] b) {
        setMsgData(VNC_SERVER_ID, MSG_FROM_VNC_CLIENT, 0, b);
        writeMessage();
    }

    /************************************************************/


    /********************** Storage calls ************************/
    public void storeObject(String name, byte [] obj) {
        int idx;
        assert(name.indexOf(':') == -1);
        name += ":";
        byte [] b = new byte[obj.length + name.length()];
        for(idx = 0; idx < name.length(); idx++) {
            b[idx] = (byte) name.charAt(idx);
        }
        for(idx = 0; idx < obj.length; idx++) {
            b[idx+name.length()] = obj[idx];
        }

        setMsgData(STORAGE_ID, MSG_STORE_OBJECT, 0, b);
        writeMessage();        
    }

    public void retrvObject(int objectId, String name, int owner) {
        assert(name.indexOf(':') == -1);
        name += ":" + owner;
        retrvObject(objectId, name);
    }
    
    public void retrvObject(int objectId, String name) {
        setMsgData(STORAGE_ID, MSG_RETRV_OBJECT, objectId, name.getBytes());
        writeMessage();
    }

    private int getColonIdx(int numColon) {
        int idx;

        for(idx = 0; idx < msgData.length; idx++) {
            if(msgData[idx] == ':') {
                numColon--;
            }

            if(numColon <= 0) {
                break;
            }
        }

        return idx;
    }

    /**
     * this extracts out the object data from an retrvObject message
     */
    public byte[] getObjectData() {
        int idx, i;
        byte[] objData;
        assert(getMsgType() == MSG_RETRV_OBJECT);
        assert(getSrcId() == STORAGE_ID);

        idx = getColonIdx(2);
        idx++; // move it beyond the second colon

        assert(idx < msgData.length);
        objData = new byte[msgData.length - idx];
        for(i = 0; i < objData.length; i++) {
            objData[i] = msgData[idx++];
        }
        assert(i == objData.length);
        assert(idx == msgData.length);        

        return objData;
    }
    
    public String getObjectId() {
        String objectId = null;

        if(getMsgType() == MSG_RETRV_OBJECT) {
            assert(getSrcId() == STORAGE_ID);
            int idx = getColonIdx(2);
            objectId = new String(msgData, 0, idx);
        } else if(getMsgType() == MSG_STORE_OBJECT) {
            assert(getDstId() == STORAGE_ID);
            int idx = getColonIdx(1);
            objectId = new String(msgData, 0, idx+1) + header.srcId;
        } else {
            assert(false);
        }

        return objectId;
    }

    /********************** UI calls ****************************/

    /***********************************************************/

    
    /*********************** Extension calls *******************/
    public void extensionAction(int actionId) {
        setMsgData(EXTENSION_ID, MSG_EXTENSION_ACTION, actionId);
        writeMessage();
    }
    /***********************************************************/

    private void setMsgData(int dstId, int msgType, int msgValue, byte [] b) {
        header.dstId = dstId;
        header.msgType = msgType;
        header.msgValue = msgValue;
        header.dataLen = (b == null) ? 0 : b.length;

        msgData = b;
    }

    private void setMsgData(int dstId, int msgType, int msgValue, String data) {
        byte [] b;
        b = (data.length() == 0) ? null : data.getBytes();
        setMsgData(dstId, msgType, msgValue, b);
    }
    
    private void setMsgData(int dstId, int msgType, int msgValue) {
        byte [] b = null;
        setMsgData(dstId, msgType, msgValue, b);
    }

    public byte [] getMsgData() {
        return msgData;
    }

    // note: read and write can have separate locks
    public void writeMessage() {
        writeLock.lock();

        header.writeHeader(out);        
        try {
            if(header.dataLen > 0) {
                out.write(msgData);
            }
            out.flush();
        } catch (IOException ioe) {
            ioe.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }

        writeLock.unlock();

    }

    private boolean llReadMessage() {
        if(!header.readHeader(in)) {
            return false;
        }
        msgData = null;
        if(header.dataLen > 0) {
            msgData = new byte[header.dataLen];
            
            try {
                in.readFully(msgData);
            } catch(Exception e) {
                e.printStackTrace();
                return false;
            }
        }
        return true;
    }

    // fixme -- syncronize
    public boolean readMessage() {
        return llReadMessage();
    }

    public long getMsgId() {
        return header.msgId;
    }

    public int getMsgValue() {
        return header.msgValue;
    }

    public void setMsgValue(int val) {
        header.msgValue = val;
    }

    public int getMsgType() {
        return header.msgType;
    }

    public int getDataLen() {
        return header.dataLen;
    }
    
    public int getDstId() {
        return header.dstId;
    }

    public boolean isDstWebApp() {
        return header.dstId >= WEBAPP_FIRST_ID;
    }

    public int getSrcId() {
        return header.srcId;
    }

    public boolean isSrcWebApp() {
        return header.srcId >= WEBAPP_FIRST_ID;
    }

    public String getStringData() {
        return (msgData == null) ? new String("") : new String(msgData);
    }

    /**
     * this will not affect the current message or the data within it,
     * it is simply a shortcut to reply to whoever sent the message
     */
    public void sendReply(byte[] replyData, int msgType) {
        MessageHeader h = new MessageHeader();                
	
        h.srcId = header.dstId;
        h.dstId = header.srcId;
        h.msgId = header.msgId;
        h.msgType = msgType;
        h.msgValue = header.msgValue;
        h.dataLen = replyData.length;

        writeLock.lock();

        h.writeHeader(out);

        try {
            if(replyData.length > 0) {
                out.write(replyData);
            }
            out.flush();
        } catch (IOException ioe) {
            ioe.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
        writeLock.unlock();

    }
    
    public Dimension getWindowSize() {
        /*
        assert(header.msgType == MSG_UPDATE_WINDOW_SIZE);
        Dimension d = new Dimension();
        
        StringTokenizer tok = new StringTokenizer(getStringData(), ":");
        assert(tok.countTokens() == 2);

        d.width = Integer.parseInt(tok.nextToken());
        d.height = Integer.parseInt(tok.nextToken());

        return d;
        */
        assert(false);
        return null;
    }


    public Rectangle getPixelClip() {
        /*
        StringTokenizer tok = new StringTokenizer(getStringData(), ":");
        Rectangle rect = new Rectangle();
        assert((header.msgType == MSG_UPDATE_PIXELS) ||
               (header.msgType == MSG_REQUEST_PIXELS));
        assert(tok.countTokens() == 4);
        
        rect.x = Integer.parseInt(tok.nextToken());
        rect.y = Integer.parseInt(tok.nextToken());
        rect.width = Integer.parseInt(tok.nextToken());
        rect.height = Integer.parseInt(tok.nextToken());

        return rect;
        */
        assert(false);
        return null;
    }

    public static String getUrlFromData(String data) {
        String url = null;

        if(data.startsWith("POST:")) {
            assert(data.length() > 5);
            int idx = data.indexOf('|');
            assert(idx >= 0);
            url = data.substring(5, idx);
        } else {
            url = data;
        }

        return url;
    }

    public static String getPostDataFromData(String data) {
        String postData = null;

        if(data.startsWith("POST:")) {
            assert(data.length() > 5);
            int idx = data.indexOf('|');
            assert(idx >= 0);
	    int idx2 = data.lastIndexOf('|');
	    assert(idx2 > idx);
            postData = data.substring(idx+1, idx2);
        }

        return postData;
    }

    public static String getPostContentTypeFromData(String data) {
	String postContentType = null;

	if(data.startsWith("POST:")) {
            assert(data.length() > 5);
            int idx = data.indexOf('|');
            assert(idx >= 0);
	    int idx2 = data.lastIndexOf('|');
	    assert(idx2 > idx);
            postContentType = data.substring(idx2 + 1);
        }

	return postContentType;
    }

    public class MessageHeader {
        public int srcId;
        public int dstId;
        public long msgId;
        public int msgType;
        public int msgValue;
        public int dataLen;

        public MessageHeader() {

        } 

        public boolean readHeader(DataInputStream din) {
            try {
                srcId = din.readInt();
                dstId = din.readInt();
                msgId = din.readLong();
                msgType = din.readInt();
                msgValue = din.readInt();
                dataLen = din.readInt();
            } catch (EOFException eof) {
                return false;
            } catch (IOException ioe) {
                ioe.printStackTrace();
                return false;
            }
            
            return true;
        }        

        public void writeHeader(DataOutputStream dout) {
            try {
                dout.writeInt(srcId);
                dout.writeInt(dstId);
                dout.writeLong(msgId);
                dout.writeInt(msgType);
                dout.writeInt(msgValue);
                dout.writeInt(dataLen);
            } catch (IOException ioe) {
                ioe.printStackTrace();
            }
        }
    }
}
