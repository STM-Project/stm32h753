#!/usr/bin/env python3
"""Exercise production ESP transmit cursors, including the entire /nowa page."""
from pathlib import Path
import test_http_sensor_routes as sensor
ROOT=Path(__file__).resolve().parents[1]
STUBS=r'''
typedef int ARCHIVING_TYPE;
#define noArch 0
#define HAL_OK 0
#define HAL_BUSY 2
#define ETH_PACKET_LEN 2039
#define DbgVarDma(...) ((void)0)
static char sendBuff[2048];
static unsigned commands,payloads,closes,command_fail,payload_fail;
static size_t sent;
static const char *expected;
static int SendToEsp32_http(int size,char *data,ARCHIVING_TYPE arch) {
    (void)data;(void)arch;assert(size>0 && (unsigned)size<sizeof sendBuff);
    ++commands;
    unsigned channel=999,length=0;int consumed=0;
    if(strstr(sendBuff,"CIPCLOSE")){
        assert(sscanf(sendBuff,"AT+CIPCLOSE=%u\r\n%n",&channel,&consumed)==1);
        ++closes;
    }else{
        assert(sscanf(sendBuff,"AT+CIPSEND=%u,%u\r\n%n",&channel,&length,&consumed)==2);
        assert(length>0 && length<=ETH_PACKET_LEN);
        int slot=-1;for(unsigned i=0;i<ESP_MAX_HTTP_CONN;i++)if(httpPar.que[i]==channel)slot=(int)i;
        assert(slot>=0 && length==httpPar.len[slot]);
    }
    assert(channel<ESP_MAX_HTTP_CONN && consumed==size);
    return command_fail?HAL_BUSY:HAL_OK;
}
static int SendToEsp32(int size,char *data,ARCHIVING_TYPE arch) {
    (void)data;(void)arch;
    assert(size>0 && size<=ETH_PACKET_LEN && (unsigned)size<sizeof sendBuff);
    if(payload_fail)return HAL_BUSY;
    assert(expected && !memcmp(sendBuff,expected,(size_t)size));
    sent+=(size_t)size;++payloads;return HAL_OK;
}
'''
TESTS=r'''
int main(void) {
    clear_queue();request(0,"/nowa");httpPar.chnl=0xFF;
    size_t total=0;for(unsigned p=0;p<MAX_HTML_WEBs && httpPar.web[0][p];p++)total+=httpPar.siz[0][p];
    assert(total>500000);sent=0;
    HTTP_SendCloseChnl(noArch,-1);
    for(unsigned guard=0;guard<1000 && !closes;guard++){
        assert(httpTxPhase[0]==1);
        unsigned part=httpPar.nrWeb[0];uint32_t offset=httpTxOffset[0];
        expected=httpPar.web[0][part]+offset;
        assert(!HTTP_SendAcknowledged(noArch)); /* premature SEND OK */
        assert(HTTP_SendPayload(noArch));
        uint32_t advanced=httpTxOffset[0];
        assert(!HTTP_SendPayload(noArch)); /* duplicate prompt */
        assert(httpTxOffset[0]==advanced);
        assert(HTTP_SendAcknowledged(noArch));
        if(!closes)assert(!HTTP_SendAcknowledged(noArch));
    }
    assert(closes==1 && sent==total && payloads>240);
    assert(!HTTP_SendPayload(noArch));assert(!HTTP_SendAcknowledged(noArch));
    /* Old code could read NULL or underflow length after a delayed prompt. */
    for(unsigned bad=0;bad<5;bad++){
        request(0,"/nowa");httpPar.chnl=0xFF;HTTP_SendCloseChnl(noArch,-1);
        if(bad==0)httpPar.ptr[0]=NULL;
        if(bad==1)httpPar.len[0]=UINT32_MAX;
        if(bad==2)httpPar.nrWeb[0]=MAX_HTML_WEBs;
        if(bad==3)httpTxOffset[0]=UINT32_MAX;
        if(bad==4)httpPar.ptr[0]++;
        assert(!HTTP_SendPayload(noArch));
    }
    /* Timeout reset is processed by the owner task; a late prompt is ignored. */
    request(0,"/nowa");httpPar.chnl=0xFF;HTTP_SendCloseChnl(noArch,-1);
    InitStructRqstToSendChnl();assert(!HTTP_SendPayload(noArch));assert(!HTTP_SendAcknowledged(noArch));
    /* HAL_BUSY must not consume payload bytes or advance a fragment. */
    request(0,"/nowa");httpPar.chnl=0xFF;command_fail=1;HTTP_SendCloseChnl(noArch,-1);
    assert(httpTxOffset[0]==0 && httpTxPhase[0]==0 && httpPar.chnl==0xFF);command_fail=0;
    HTTP_SendCloseChnl(noArch,-1);payload_fail=1;assert(!HTTP_SendPayload(noArch));payload_fail=0;
    assert(httpTxOffset[0]==0 && httpTxPhase[0]==0);
    /* Exact packet multiples and empty fragments must end without a zero send. */
    clear_queue();static char exact[ETH_PACKET_LEN*2];
    httpPar.que[0]=0;httpPar.chnl=0xFF;httpTxOffset[0]=0;httpTxPhase[0]=0;
    httpPar.web[0][0]=exact;httpPar.siz[0][0]=sizeof exact;
    httpPar.web[0][1]=exact;httpPar.siz[0][1]=0;
    httpPar.web[0][2]=NULL;closes=0;sent=0;
    HTTP_SendCloseChnl(noArch,-1);
    for(unsigned n=0;n<2;n++){expected=exact+sent;assert(HTTP_SendPayload(noArch));assert(HTTP_SendAcknowledged(noArch));}
    assert(closes==1 && sent==sizeof exact);
    /* Interleave long pages and a live JSON reply on three ESP channels. */
    clear_queue();HTTP_HandleTarget(3,"/nowa",5,NULL);HTTP_HandleTarget(1,"/api/nowa",9,NULL);HTTP_HandleTarget(4,"/nowa",5,NULL);
    total=0;for(unsigned q=0;q<3;q++)for(unsigned p=0;p<MAX_HTML_WEBs && httpPar.web[q][p];p++)total+=httpPar.siz[q][p];
    httpPar.chnl=0xFF;closes=0;sent=0;HTTP_SendCloseChnl(noArch,-1);
    for(unsigned guard=0;guard<2000 && closes<3;guard++){
        int slot=SearchChnl(httpPar.chnl);assert(slot>=0);
        if(httpTxPhase[slot]==3){httpPar.que[slot]=0xFF;httpPar.ptr[slot]=NULL;httpTxPhase[slot]=0;httpPar.chnl=0xFF;HTTP_SendCloseChnl(noArch,-1);continue;}
        expected=httpPar.web[slot][httpPar.nrWeb[slot]]+httpTxOffset[slot];
        assert(HTTP_SendPayload(noArch));assert(HTTP_SendAcknowledged(noArch));
    }
    assert(closes==3 && sent==total);
    /* Stream distinct SD pages across interleaved ESP channels. */
    static char sd1[17001],sd2[23001];
    for(unsigned i=0;i<sizeof sd1-1;i++)sd1[i]=(char)('a'+i%26);
    for(unsigned i=0;i<sizeof sd2-1;i++)sd2[i]=(char)('A'+i%26);
    lpc_test_page=sd1;lpc_test_page2=sd2;
    clear_queue();HTTP_HandleTarget(0,"/nowa1",6,NULL);HTTP_HandleTarget(4,"/nowa2",6,NULL);
    total=0;for(unsigned q=0;q<2;q++)for(unsigned p=0;p<MAX_HTML_WEBs&&httpPar.web[q][p];p++)total+=httpPar.siz[q][p];
    closes=0;sent=0;httpPar.chnl=0xFF;HTTP_SendCloseChnl(noArch,-1);
    for(unsigned guard=0;guard<100&&closes<2;guard++){
        int slot=SearchChnl(httpPar.chnl);assert(slot>=0);
        if(httpTxPhase[slot]==3){httpPar.que[slot]=0xFF;httpPar.ptr[slot]=NULL;httpPar.chnl=0xFF;HTTP_SendCloseChnl(noArch,-1);continue;}
        unsigned part=httpPar.nrWeb[slot];
        expected=(part==1?test_sd_page(httpLpcPage[slot]):httpPar.web[slot][part])+httpTxOffset[slot];
        assert(HTTP_SendPayload(noArch));assert(HTTP_SendAcknowledged(noArch));
    }
    assert(closes==2&&sent==total);
    /* A removed/unreadable SD page closes before announcing its next packet. */
    clear_queue();request(0,"/nowa1");httpPar.chnl=0xFF;closes=0;
    HTTP_SendCloseChnl(noArch,-1);expected=httpPar.web[0][0];assert(HTTP_SendPayload(noArch));
    lpc_chunk_fail=1;assert(HTTP_SendAcknowledged(noArch));
    assert(closes==1&&httpTxPhase[0]==3&&!HTTP_SendPayload(noArch));lpc_chunk_fail=0;
    /* Force the real JSON window fallback without a large staging allocation. */
    static char jsonExpected[100000];
    size_t jsonLength=LPC_SettingsJson(1,jsonExpected,sizeof jsonExpected);assert(jsonLength);
    lpc_force_json_stream=1;clear_queue();request(0,"/api/lpc1");lpc_force_json_stream=0;
    assert(httpLpcPage[0]&HTTP_LPC_JSON_STREAM);
    closes=0;sent=0;httpPar.chnl=0xFF;HTTP_SendCloseChnl(noArch,-1);
    size_t jsonTotal=httpPar.siz[0][0]+jsonLength;
    for(unsigned guard=0;guard<100&&!closes;guard++){
        unsigned part=httpPar.nrWeb[0];expected=(part==1?jsonExpected:httpPar.web[0][part])+httpTxOffset[0];
        assert(HTTP_SendPayload(noArch));assert(HTTP_SendAcknowledged(noArch));
    }
    assert(closes==1&&sent==jsonTotal);
    puts("PASS: real JSON window fallback preserves serialized content and response length.");
    puts("PASS: interleaved SD streams preserve page/offset; midstream SD failure closes without stale payload.");
    puts("PASS: full 500KB page, exact packet boundaries, empty fragments, duplicate/late prompts and acknowledgements, timeout reset, corrupted cursors and DMA rejection (ASan/UBSan).");
    return 0;
}
'''
def main():
    source=(ROOT/'Application/esp32wroom.c').read_text()
    init=source[source.index('static void InitStructRqstToSendChnl('):source.index('static int IsActiveAnyRqstToSendChnl(')]
    tx=source[source.index('int CheckReadyToSendChnl ('):source.index('static int HTTP_GetClosedChannel(')]
    # Guard against reintroducing a timer-task mutation of the HTTP queue.
    callback=source[source.index('static void vWaitOnSendTimeoutTimerCallback('):source.index('static int RcvTimeoutTimerInit(')]
    assert 'InitStructRqstToSendChnl(' not in callback
    assert 'httpResetRequested = 1' in callback
    base=sensor.TESTS.replace('int main(void)','static int __attribute__((unused)) sensor_routes_suite(void)')
    sensor.compile_and_run(sensor.harness_source(base+STUBS+init+tx+TESTS))
if __name__=='__main__':main()
