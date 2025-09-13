#include"ConnectionPool.h"

ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}
bool ConnectionPool::parseJsonFile()
{
	ifstream ifs("dbconf.json");
	Reader rd;
	Value root;
	rd.parse(ifs, root);//写到root里

    if (root.isObject())
    {
        m_ip = root["ip"].asString();
        m_port = root["port"].asInt();
        m_user = root["userName"].asString();
        m_passwd = root["password"].asString();
        m_dbName = root["dbName"].asString();
        m_minSize = root["minSize"].asInt();
        m_maxSize = root["maxSize"].asInt();
        m_maxidletime = root["maxIdleTime"].asInt();
        m_timeout = root["timeout"].asInt();
        return true;
    }
    return false;
}

ConnectionPool::ConnectionPool()
{
    //加载配置
    if (!parseJsonFile()) return;
   /* cout << "开始初始化\n";*/
    for (int i = 0; i < m_minSize; i++)
    {
        addConnection();
        /*cout << "生成第 " << i << " 个连接\n";*/
    }

    thread producer(&ConnectionPool::produceConnection,this);
    thread remove(&ConnectionPool::removeConnection, this);

    producer.detach();//处理连接不足
    remove.detach();//处理空闲过久的连接
}

void ConnectionPool::produceConnection()
{
    while (true)
    {
        unique_lock<mutex> lock(m_mtx);
        while (m_connections.size() >= m_minSize)
        {
            m_conditon.wait(lock);
        }
        addConnection();
        m_conditon.notify_all();
    }
}

void ConnectionPool::removeConnection()
{
    while (true)
    {
        this_thread::sleep_for(chrono::seconds(1));
        unique_lock<mutex> lock(m_mtx);
        while (m_connections.size() >= m_minSize)
        {

            MysqlConn* conn = m_connections.front();
            if (conn->getAliveTime() >= m_maxidletime)
            {
                m_connections.pop();
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}

void ConnectionPool::addConnection()
{
    /*cout << "开始创造\n";*/
    MysqlConn* conn = new MysqlConn;
   /* cout << "创造成功\n";*/
    int ret=conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    if (!ret)cout << "连接失败 "<<ret<<"\n";
  /*  cout << "创造成功\n";*/
    conn->refreshTime();
   /* cout << "创造成功\n";*/
    m_connections.push(conn);
    //cout << "创造成功\n";
}

shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    unique_lock<mutex>lock(m_mtx);
    while (m_connections.empty())
    {
        if (cv_status::timeout == m_conditon.wait_for(lock, chrono::milliseconds(m_timeout)))
        {
            if (m_connections.empty())
            {
                continue;
            }
        }//如果超时就继续等待，否则就取走
    }
    shared_ptr<MysqlConn> connptr(m_connections.front(), [this](MysqlConn* conn) {
        lock_guard<mutex>lg(m_mtx);
        m_connections.push(conn);
        conn->refreshTime();
        });//写删除触发的回调函数;
    m_connections.pop();
	m_conditon.notify_all();//提醒生产者线程检查连接池总数是否小于min值

    return connptr;
}

ConnectionPool::~ConnectionPool()
{
    while (!m_connections.empty())
    {
        MysqlConn* conn = m_connections.front();
        m_connections.pop();
    }
}