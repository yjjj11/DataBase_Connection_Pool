#include<iostream>
#include<memory>
#include"MysqlConn.h"
#include"ConnectionPool.h"
using namespace std;

void op1(int begin, int end)
{
	for (int i = begin; i < end; i++)
	{
		MysqlConn conn;
		int ret = conn.connect("root", "1438354393", "test", "127.0.0.1");
		if (!ret)cout << "连接错误\n";
		char sql[1024] = { 0 };
		sprintf_s(sql,"insert into person values(%d,18,'man','zyt')",i);
		int flag = conn.update(sql);
		/*cout << "flag value: " << flag << "\n";*/
	}
}

void op2(ConnectionPool* pool, int begin, int end)
{
	for (int i = begin; i < end; i++)
	{
		shared_ptr<MysqlConn> conn = pool->getConnection();
		/*cout << "取出第 " << i << " 个连接\n";*/
		char sql[1024] = { 0 };
		sprintf_s(sql, "insert into person values(%d,18,'man','zyt')", i);
		int flag = conn->update(sql);
	}
}
int query()
{
	MysqlConn conn;
	int ret=conn.connect("root", "1438354393", "test", "127.0.0.1");
	if (!ret)cout << "连接错误\n";
	string sql = "insert into person values(6,18,'man','zyt')";
	int flag=conn.update(sql);
	cout << "flag value: " << flag << "\n";

	sql = "select * from person";
	conn.query(sql);
	while (conn.next())//当还有数据的时候
	{
		cout << conn.value(0) << ", " << conn.value(1) << ", "<<conn.value(2) << ", " << conn.value(3) << "\n";
	}
	return 0;
}


void test1()
{
	auto begin = steady_clock::now();
	op1(0, 5000);
	auto end = steady_clock::now();
	auto length = end - begin;
	cout << "非连接池，单线程用时" << length.count() << "纳秒, " << length.count() / 1000000 << "毫秒\n";
}
void test2()
{
	auto pool = ConnectionPool::getConnectionPool();
	auto begin = steady_clock::now();
	op2(pool,0,5000);
	auto end = steady_clock::now();
	auto length = end - begin;
	cout << "连接池，单线程用时" << length.count() << "纳秒, " << length.count() / 1000000 << "毫秒\n";
}
void test3()
{
	MysqlConn conn;
	int ret = conn.connect("root", "1438354393", "test", "127.0.0.1");
	steady_clock::time_point begin = steady_clock::now();
	thread t1(op1, 0, 1000);
	thread t2(op1, 1000, 2000);
	thread t3(op1, 2000, 3000);
	thread t4(op1, 3000, 4000);
	thread t5(op1, 4000, 5000);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "非连接池，多线程，用时：" << length.count() << " 纳秒，"
		<< length.count() / 1000000 << " 毫秒" << endl;
}
void test4()
{
	ConnectionPool* pool = ConnectionPool::getConnectionPool();
	steady_clock::time_point begin = steady_clock::now();
	thread t1(op2, pool, 0, 1000);
	thread t2(op2, pool, 1000, 2000);
	thread t3(op2, pool, 2000, 3000);
	thread t4(op2, pool, 3000, 4000);
	thread t5(op2, pool, 4000, 5000);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "非连接池，多线程，用时：" << length.count() << " 纳秒，"
		<< length.count() / 1000000 << " 毫秒" << endl;
}
int main()
{
	/*test1();*/
	
	/*test2(); */
	/*query();*/
	/*test3();*/
	test4();
	return 0;
}