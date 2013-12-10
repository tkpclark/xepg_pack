#define DST_CMD_OFFS 0
#define DST_CLTID_OFFS 4
#define DST_CLTKEY_OFFS 8
#define DST_DEVID_OFFS 12
#define DST_CHNID_OFFS 16
#define DST_INTERVAL_OFFS 20
#define DST_TS_LEN_OFFS 24
#define DST_STREAM_OFFS 28
#define DST_BASE_OFFS 30
#define DST_TS_BUF_OFFS 34
#define DST_CMD1_BODY_OFFS DST_DEVID_OFFS

#define DST_CODE_OFFS 0

#define DST_HDR_LEN 12


typedef struct
{
	char *buffer;
	int len;
}DST_STREAM;

typedef struct
{
	int cmd;
	int cltid;
	int cltkey;
}DST_HDR;

typedef struct
{
	DST_HDR *p_dst_hdr;
	int DeviceId;
	int ChannelId;
	unsigned short stream_id;
	int base_id;
	int interval;
	int ts_len;
	char *ts_buffer;
}DST_CMD1;

typedef struct
{
	DST_HDR *p_dst_hdr;

}DST_CMD10;

typedef struct
{
	DST_HDR *p_dst_hdr;

}DST_CMD2;

typedef struct
{
	int code;
}DST_RESP;

typedef struct
{
	char *buffer;
	unsigned int len;
}TS_BUFFER_INFO;

void DST_com_header(DST_HDR *p_dst_hdr,DST_STREAM *p_dst_stream);
void DST_parse_header(DST_HDR *p_dst_hdr,DST_STREAM *p_dst_stream); 
void DST_com_cmd1(DST_CMD1 *p_dst_cmd1,DST_STREAM *p_dst_stream);
void DST_parse_cmd1(DST_CMD1 *p_dst_cmd1,DST_STREAM *p_dst_stream);
void DST_parse_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream);
int DST_send_data(DST_STREAM *p_dst_stream,char *dip,short dport);
void DST_com_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream);
void DST_parse_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream);



