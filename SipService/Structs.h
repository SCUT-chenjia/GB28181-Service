#pragma once
#include "pch.h"

struct SipServerInfo {
	std::string IP;
	int Port;
	std::string ID;
	std::string Realm;
	std::string Password;
	std::string Nonce;
	std::string ExternIP;
};

struct MediaServerInfo {
	std::string IP;
	int Port;
	std::string Secret;
};


enum class PresetCommand
{
	SET = 129,
	CALL = 130,
	DEL = 131
};


enum class PtzCommand :int
{
	LEFT,
	RIGHT,
	UP,
	DOWN,
	UPLEFT,
	UPRIGHT,
	DOWNLEFT,
	DOWNRIGHT,
	ZOOMIN,
	ZOOMOUT,
	STOP
};

enum REQUEST_MESSAGE_TYPE
{
	REQUEST_TYPE_UNKNOWN = 0,
	KEEPALIVE,                   // ��������
	QUERY_CATALOG,               //   ��ѯĿ¼
	QUERY_DEVICEINFO,               //   ��ѯĿ¼
	DEVICE_CONTROL_PTZ,         // �豸����-��̨
	DEVICE_QUERY_PRESET,        // �豸��ѯ-Ԥ��λ
	DEVICE_CONTROL_PRESET,       // �豸����-Ԥ��λ
	DEVICE_CONTROL_HOMEPOSITION, // �豸����-����λ

	REQUEST_CALL_INVITE,            // �㲥
	REQUEST_CALL_PLAYBACK,          // �ط�
	REQUEST_CALL_LIVE,              // ֱ��
	REQUEST_CALL_DOWNLOAD,          // ����
	REQUEST_CALL_BYE,               // �Ҷ�

	REQUEST_TYPE_MAX
};


enum REQ_CALL_TYPE
{
	REQ_CALL_TYPE_UNKNOWN = 0,
	REQ_CALL_TYPE_MAX
};


enum manscdp_cmd_category_e
{
	MANSCDP_CMD_CATEGORY_CONTROL,
	MANSCDP_CMD_CATEGORY_QUERY,
	MANSCDP_CMD_CATEGORY_NOTIFY,
	MANSCDP_CMD_CATEGORY_RESPONSE,

	MANSCDP_CMD_CATEGORY_MAX,
	MANSCDP_CMD_CATEGORY_UNKNOWN = MANSCDP_CMD_CATEGORY_MAX
};

enum manscdp_cmdtype_e
{
	//< Control
	MANSCDP_CONTROL_CMD_DEVICE_CONTROL,      ///<�豸����
	MANSCDP_CONTROL_CMD_DEVICE_CONFIG,       ///<�豸����

	//< Query
	MANSCDP_QUERY_CMD_DEVICE_STATUS,        ///<�豸����
	MANSCDP_QUERY_CMD_CATALOG,              ///<�豸Ŀ¼��ѯ
	MANSCDP_QUERY_CMD_DEVICE_INFO,          ///<�豸��Ϣ��ѯ
	MANSCDP_QUERY_CMD_RECORD_INFO,          ///<�ļ�Ŀ¼����
	MANSCDP_QUERY_CMD_ALARM,                ///<������ѯ
	MANSCDP_QUERY_CMD_CONFIG_DOWNLOAD,      ///<�豸���ò�ѯ
	MANSCDP_QUERY_CMD_PRESET_QUERY,         ///<Ԥ��λ��ѯ
	MANSCDP_QUERY_CMD_MOBILE_POSITION,      ///<�ƶ��豸λ�����ݲ�ѯ

	//< Notify
	MANSCDP_NOTIFY_CMD_KEEPALIVE,           ///<�豸״̬��Ϣ���ͣ�����
	MANSCDP_NOTIFY_CMD_ALARM,               ///<����֪ͨ
	MANSCDP_NOTIFY_CMD_MEDIA_STATUS,        ///<ý��֪ͨ
	MANSCDP_NOTIFY_CMD_BROADCASE,           ///<�����㲥֪ͨ
	MANSCDP_NOTIFY_CMD_MOBILE_POSITION,     ///<�ƶ��豸λ��֪ͨ

	//< Response
	MANSCDP_RESOPNSE_CMD_DEVICE_CONTROL,    ///<�豸������Ӧ
	MANSCDP_RESOPNSE_CMD_DEVICE_CONFIG,     ///<�豸������Ӧ
	MANSCDP_RESOPNSE_CMD_DEVICE_STATUS,     ///<�豸״̬��ѯ��Ӧ
	MANSCDP_RESOPNSE_CMD_DEVICE_CATALOG,    ///<�豸Ŀ¼��ѯ��Ӧ


	MANSCDP_CMD_TYPE_MAX,
	MANSCDP_CMD_TYPE_UNKNOWN = MANSCDP_CMD_TYPE_MAX
};

enum manscdp_devicecontrol_subcmd_e
{
	PTZCmd = 1,
	TeleBoot,
	RecordCmd,
	GuardCmd,
	AlarmCmd = 5,
	IFrameCmd,
	DragZoomIn,
	DragZoomOut,
	HomePosition
};

enum manscdp_switch_status_e
{
	SWITCH_ON,          ///< ״̬�л�  ����
	SWITCH_OFF
};

enum manscdp_deviceconfig_subcmd_e
{
	BasicParam = 1,
	SVACEncodeConfig,
	SVACDecodeConfig
};

typedef std::vector<manscdp_devicecontrol_subcmd_e> manscdp_devicecontrol_subcmd_t;
typedef  std::vector<manscdp_deviceconfig_subcmd_e> manscdp_deviceconfig_subcmd_t;
typedef  std::vector< std::string> manscdp_configdownload_subcmd_t;

//typedef tinyxml2::XMLDocument   tinyxml_doc_t;
//typedef tinyxml2::XMLElement    tinyxml_ele_t;

///< MANSCDP xml��Ϣ ��ѡ������
/*
 * <?xml version="1.0"?>
 * <Query>
 * <CmdType>Catalog</CmdType>
 * <SN>27</SN>
 * <DeviceID>1234</DeviceID>
 * </Query>
 */
struct manscdp_msgbody_header_t
{
	manscdp_cmd_category_e              cmd_category;
	manscdp_cmdtype_e                   cmd_type;
	std::string                         sn;
	std::string                         devid;
	manscdp_deviceconfig_subcmd_t       devcfg_subcmd;
	manscdp_devicecontrol_subcmd_t      devctl_subcmd;
	manscdp_configdownload_subcmd_t     cfgdownload_subcmd;
};


enum deviceid_type_e
{

};

enum status_type_e
{
	STATUS_UNKNOWN = 0,
	ON,
	OFF,
};


enum result_type_e
{
	RESULT_UNKNOWN = 0,
	OK,
	Error
};

enum ptz_type_e
{

};

enum record_type_e
{
	RECORD_UNKNOWN = 0,
	RECORD,
	STOP_RECORD
};

enum guard_type_e
{
	GUARD_UNKNON = 0,
	SET_GUARD,
	RESET_GUARD
};

enum item_type_e
{

};

enum item_file_type_e
{

};


struct common_item_t
{
	std::string      cmdtype;
	std::string      sn;
	std::string      dev_id;
};

///< ͨ��Ӧ���ʽ
///< Broadcast��DeviceConfig��Catalog(Ŀ¼�յ�Ӧ��)��Alarm��DeviceControl
struct common_answer_t : public common_item_t
{
	result_type_e   result;
};

struct manscdp_basicparam_cfg_t
{
	uint32_t    expire;
	uint32_t    heartbeat_interval;
	uint32_t    heartbeat_cnt;
	std::string      dev_name;
};


struct manscdp_device_config_dialogs_t
{
	struct request_t : public common_item_t {
	public:
		request_t() {
			cmdtype = "DeviceConfig";
		}
		manscdp_basicparam_cfg_t        basicparam;
		///TODO add SVACEncodeConfig so and on.
	};
	struct response_t : public common_answer_t {
	public:
		response_t() {
			cmdtype = "DeviceConfig";
		}
	};

public:
	request_t           request;
	response_t          response;
};


struct manscdp_keepalive_dialogs_t
{
	struct request_t : public common_item_t {
		struct info {
			std::string      devid;
		};
	public:
		request_t() {
			cmdtype = "Keepalive";
		}
		result_type_e       result;
		std::list<info>          error_devlist;
	};
	struct response_t : public common_answer_t {
		response_t() {
			cmdtype = "Keepalive";
		}
	};
public:
	request_t       request;
	response_t      response;
};

struct manscdp_alarm_notify_dialogs_t
{
	struct request_t : public common_item_t {
		struct info {
			struct param {
				uint32_t        eventtype;
			};
		public:
			uint32_t        alarmtype;
			param           alarmtype_param;
		};
	public:
		request_t() {
			cmdtype = "Alarm";
		}
		std::string              alarm_priority;
		std::string              alarm_method;
		std::string              alarmtime;
		std::string              alarm_description;
		double              longitude;
		double              latitude;
		std::list<info>          info_list;
	};
	struct response_t : public common_answer_t {
		response_t() {
			cmdtype = "Alarm";
		}
	};

public:
	request_t           request;
	response_t          response;
};

struct manscdp_media_status_dialogs_t
{
	struct request_t : public common_item_t {
	public:
		request_t() {
			cmdtype = "MediaStatus";
		}
		std::string      notifytype;
	};
	struct response_t : public common_answer_t {
	public:
		response_t() {
			cmdtype = "MediaStatus";
		}
	};

public:
	request_t       request;
	response_t      response;
};

struct manscdp_broadcast_dialogs_t
{
	struct request_t : public common_item_t {
	public:
		request_t() {
			cmdtype = "Broadcast";
		}
		std::string          sourceid;
		std::string          targetid;
	};
	struct response_t : public common_answer_t {
	public:
		response_t() {
			cmdtype = "Broadcast";
		}
	};

public:
	request_t       request;
	response_t      response;
};



struct manscdp_device_status_dialog_t
{
	struct request_t : public common_item_t {
		request_t() {
			cmdtype = "DeviceStatus";
		}
	};
	struct response_t : public common_item_t {
		struct item {
			std::string         devid;
			std::string         dutystatus;
		};
	public:
		response_t() {
			cmdtype = "DeviceStatus";
		}
		result_type_e      result;
		result_type_e      work_status;         ///<�Ƿ���������
		status_type_e      encode;
		status_type_e      record;
		std::string             dev_time;
		std::string             onoff_line;
		std::string             reason;
		std::vector<item>       alarm_status;
	};

public:
	request_t   request;
	response_t  response;
};

struct manscdp_cataloginfo_dialog_t
{
	struct request_t : public common_item_t {
	public:
		request_t() {
			cmdtype = "Catalog";
		}
		std::string      startime;
		std::string      endtime;
	};
	struct response_t : public common_item_t {
		struct item {
			struct info_t {
				uint8_t         ptztype;
				uint8_t         position_type;
				uint8_t         roomtype;
				uint8_t         usetype;
				uint8_t         supplylight_type;
				uint8_t         direction_type;
				std::string          resolution;
				std::string          business_groupid;
				std::string          download_speed;
				uint8_t         svc_space_supportmode;
				uint8_t         svc_time_supportmode;
			};
		public:
			std::string      devid;          ///< must
			std::string      name;           ///< must
			std::string      manufacturer;   ///< must
			std::string      model;          ///< must
			std::string      owner;          ///< must
			std::string      civilcode;      ///< must
			std::string      block;
			std::string      address;
			uint8_t     parental;
			std::string      parentid;
			uint8_t     safetyway;
			uint8_t     register_way;
			std::string      certnum;
			uint8_t     certifiable;
			uint8_t     errcode;
			std::string      endtime;
			uint8_t     secrecty;
			std::string      ipaddress;
			uint8_t     port;
			std::string      passwd;
			status_type_e   status;
			double      longitude;
			double      latitude;
			info_t      info;
		};
	public:
		response_t() {
			cmdtype = "Catalog";
		}

		uint32_t    sum;
		std::list<item>  devlist;
	}; ///< response_t

public:
	request_t           request;
	response_t          response;
};

struct manscdp_devinfo_dialog_t
{
	struct request_t : public common_item_t {
		request_t() {
			cmdtype = "DeviceInfo";
		}
	};
	struct response_t : public common_item_t {
		std::string          devname;
		result_type_e   result;
		std::string          manufacturer;
		std::string          model;
		std::string          firmware;
		uint8_t         channel;
		response_t() {
			cmdtype = "DeviceInfo";
		}
	};

public:
	request_t       request;
	response_t      response;
};

struct manscdp_recordinfo_dialog_t
{
	struct request_t : public common_item_t {
		std::string      starttime;
		std::string      endtime;
		std::string      filepath;
		std::string      address;
		uint8_t     secrecy;
		std::string      record_type;        ///< time/alarm/manual/all
		std::string      recordid;
		std::string      indistinct_query;
	public:
		request_t() {
			cmdtype = "RecordInfo";
		}
	};
	struct response_t : public common_item_t {
		struct item {
			std::string      devid;
			std::string      devname;
			std::string      filepath;
			std::string      address;
			std::string      starttime;
			std::string      endtime;
			uint8_t     secrecy;
			std::string      type;
			std::string      recordid;
			uint32_t    filesize;
		};
	public:
		uint32_t        sum;
		uint32_t        item_num;
		std::string          name;
		std::list<item>      recordlist;

		response_t() {
			cmdtype = "RecordInfo";
		}
	};

public:
	request_t           request;
	response_t          response;
};

struct manscdp_config_download_dialog_t
{
	struct request_t : public common_item_t {
		std::string          configtype;         ///< ���� / �ָ�

	public:
		request_t() {
			cmdtype = "ConfigDownload";
		}
	};

	struct response_t : public common_item_t {
		struct basic_param {
			uint8_t                     position_capability;
			double                      longitude;
			double                      latitude;
			manscdp_basicparam_cfg_t    basic_cfg;
		};
		struct video_param_opt {
			std::string                      download_speed;
			std::string                      resultion;
		};

	public:
		response_t() {
			cmdtype = "ConfigDownload";
		}
		result_type_e   result;
		basic_param     basic;
		video_param_opt video;
	};

public:
	request_t       request;
	response_t      response;
};

struct manscdp_preset_query_dialog_t
{
	struct request_t : public common_item_t {
		request_t() {
			cmdtype = "PresetQuery";
		}
	};
	struct response_t : public common_item_t {
		struct item {
			std::string      preset_id;
			std::string      preset_name;
		};
	public:
		response_t() {
			cmdtype = "PresetQuery";
		}
		uint32_t        item_num;
		std::list<item>      preset_list;
	};

public:
	request_t           request;
	response_t          response;
};

struct manscdp_alarm_query_dialog_t
{
	struct request_t : public common_item_t {
		std::string          start_alarmpriority;
		std::string          end_alarmpriority;
		std::string          alarm_method;
		std::string          alarmtype;
		std::string          start_alarmtime;
		std::string          end_alarmtime;
	public:
		request_t() {
			cmdtype = "Alarm";
		}
	};

public:
	request_t           request;
	//TODO response
};





///< ��̨ ��ͷ�䱶
struct ptz_cmd_zoom_t
{
	enum cmd_type_e {
		ZOOM_UNKNOWN = 0,
		ZOOM_OUT,
		ZOOM_IN,
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< ��̨��ֱ�������
struct ptz_cmd_tilt_t
{
	enum cmd_type_e {
		TILT_UNKNOWN = 0,
		TILT_UP,
		TILT_DOWN,
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< ��̨ˮƽ���Ʒ���
struct ptz_cmd_pan_t
{
	enum cmd_type_e {
		PAN_UNKNOWN = 0,
		PAN_LEFT,
		PAN_RIGHT,
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< FIָ�� ��Ȧ
struct fi_cmd_iris_t
{
	enum cmd_type_e {
		IFIS_UNKNOWN = 0,
		IFIS_SHRINK,            ///<��С
		IFIS_AMPLIFICATION,     ///<�Ŵ�
	};
	cmd_type_e  cmdtype;
	uint8_t     speed;
};

///< FIָ�� �۽� ��Ȧ
struct fi_cmd_focus_t
{
	enum cmd_type_e {
		FOCUS_UNKNOWN = 0,
		FOCUS_NEAR,
		FOCUS_FAR,
	};
	cmd_type_e      cmdtype;
	uint8_t         speed;
};

///< Ԥ��λָ��
struct preset_cmd_t
{
	enum cmd_type_e {
		PRESET_UNKNOWN = 0,
		PRESET_SET,
		PRESET_CALL,
		PRESET_DELE
	};
	cmd_type_e  cmdtype;
	uint8_t     index;
};

///< Ѳ��ָ��
struct patrol_cmd_t
{
	enum cmd_type_e {
		PATROL_UNKNOWN = 0,
		PATROL_ADD,
		PATROL_DELE,
		PATROL_SET_SPEED,
		PATROL_SET_TIME,    ///<����ͣ��ʱ��
		PATROL_START,       ///<��ʼѲ��
		PATROL_STOP
	};
	cmd_type_e  cmdtype;
	uint8_t     patrol_id;      ///<Ѳ�����
	uint8_t     preset_id;      ///<Ԥ��λ��
	uint16_t    value;          ///<���ݣ��ٶȺ�ͣ��ʱ��ʹ��
};

///< �Զ�ɨ��ָ��
struct scan_cmd_t
{
	enum cmd_type_e {
		SCAN_UNKNOWN = 0,
		SCAN_START,
		SCAN_STOP,
		SCAN_SET_LEFT_BOADER,
		SCAN_SET_RIGHT_BOADER,
		SCAN_SET_SPEED
	};
	cmd_type_e  cmdtype;
	uint8_t     scan_id;
	uint16_t    speed;      ///< ����scan�ٶ�ʹ��
};

enum control_type_e
{
	CTRL_CMD_UNKNOWN = 0,
	PTZ_TYPE,       ///< PTZ����
	FI_TYPE,        ///<��Ȧ���۽�����
	PRESET_TYPE,    ///<Ԥ��λ
	PATROL_TYPE,    ///<Ѳ��
	SCAN_TYPE,      ///<ɨ��
	AUX_TYPE,       ///<��������

	CONTROL_STOP    ///<ֹͣ����
};

struct control_cmd_t
{
	uint8_t             first_byte;     ///< A5H
	uint8_t             version;        ///< �汾��
	uint8_t             check;          ///< У��λ

	control_type_e      ctrltype;

	struct {
		ptz_cmd_pan_t   ptz_pan;
		ptz_cmd_tilt_t  ptz_tilt;
		ptz_cmd_zoom_t  ptz_zoom;
	};

	struct {
		fi_cmd_focus_t  fi_focus;
		fi_cmd_iris_t   fi_iris;
	};

	preset_cmd_t        preset;

	patrol_cmd_t        patrol;

	scan_cmd_t          autoscan;
};