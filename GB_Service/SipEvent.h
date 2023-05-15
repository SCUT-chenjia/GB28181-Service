#pragma once

class SipEvent :public std::enable_shared_from_this<SipEvent>
{
public:
    typedef std::shared_ptr<SipEvent> Ptr;
    typedef std::function<int(const SipEvent::Ptr&)> event_proc;

public:
    int                 value;      // �¼�ֵ
    const char*         name;       // �¼�����
    event_proc          proc;       // �¼�������
    struct eXosip_t*    exosip_context;  // eXosip������
    eXosip_event_t*     exosip_event;    // eXosip�¼�
    uint64_t            id;         // �¼�id
};

