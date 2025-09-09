#include"MysqlConn.h"
MysqlConn::MysqlConn()
{
	m_conn = mysql_init(nullptr);
	mysql_set_character_set(m_conn, "utf8");
}
MysqlConn::~MysqlConn()
{
	if (m_conn != nullptr)
		mysql_close(m_conn);
	freeResult();
}
bool MysqlConn::connect(string user, string password, string dbname, string ip, unsigned short port)
{
	MYSQL* ptr=mysql_real_connect(m_conn, ip.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0);
	return ptr != nullptr;
}

bool MysqlConn::update(string sql)
{
	int ret=mysql_query(m_conn, sql.c_str());
	if (ret)return false;
	return true;
}

bool MysqlConn::query(string sql)
{
	freeResult();
	int ret=mysql_query(m_conn, sql.c_str());
	if (ret)return false;
	m_result=mysql_store_result(m_conn);
	return true;;
}
bool MysqlConn::next()
{
	if (m_result != nullptr)
	{
		m_row=mysql_fetch_row(m_result);
		if (m_row != nullptr)return true;
	}
	return false;
}
string MysqlConn::value(int index)
{
	int columnCount = mysql_num_fields(m_result);//�е�����
	if (index >= columnCount || index < 0)return string();
	char* val = m_row[index];

	size_t length=mysql_fetch_lengths(m_result)[index];//ÿ�еĳ�������
	return string(val,length);//�����ָ�����ȣ�����������'\0'��ͽض���
}

bool MysqlConn::transaction()
{
	return mysql_autocommit(m_conn, false);
}
bool MysqlConn::commit()
{
	return mysql_commit(m_conn);
}
bool MysqlConn::rollback()
{
	return mysql_rollback(m_conn);
}

void MysqlConn::freeResult()
{
	if (m_result==nullptr)return;
	mysql_free_result(m_result);
	m_result = nullptr;
}

//ˢ�¿���ʱ���
void MysqlConn::refreshTime()
{
	m_alivetime = steady_clock::now();
}
//ˢ�¿�����ʱ��
long long MysqlConn::getAliveTime()
{
	nanoseconds res = steady_clock::now() - m_alivetime;
	milliseconds millisec = duration_cast<milliseconds>(res);
	return millisec.count();
}