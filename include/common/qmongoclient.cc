#include "qmongoclient.h"

Q_BEGIN_NAMESPACE

QMongoClient::QMongoClient(const char* uri) :
	conn_(NULL),
	uri_(uri)
{
	Q_ASSERT(instance_.initialized(), "QMongoClient: failed to initialize the client driver!");

	cs_ = ConnectionString::parse(uri_, errmsg_);
	Q_ASSERT(cs_.isValid(), "QMongoClient: parsing connection string (%s) error for (%s)...", uri_.c_str(), errmsg_.c_str());

	conn_ = cs_.connect(errmsg_);
	Q_ASSERT(conn_, "QMongoClient: connecting to (%s) error for (%s)...", uri_.c_str(), errmsg_.c_str());
}

QMongoClient::~QMongoClient()
{
	q_delete< mongo::DBClientBase >(conn_);
}

void QMongoClient::setCollection(const char* collection)
{
	collection_ = collection;
}

const char* QMongoClient::getCollection() const
{
	return collection_.c_str();
}

int32_t QMongoClient::selectAll()
{
	try {
		std::auto_ptr<DBClientCursor> cursor = 
			conn_->query(collection_, mongo::BSONObj());
		while(cursor->more())
			Q_INFO("Data: (%s)!", cursor->next().toString().c_str());
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return MONGO_ERR;
	}
	return MONGO_OK;
}

int32_t QMongoClient::insert(const char* id, const char* idValue, const char* columnName, const char* columnValue)
{
	if(id == NULL || idValue == NULL || columnName == NULL || columnValue == NULL)
		return MONGO_ERR;

	try {
		mongo::BSONObj p = BSON(GENOID \
				<< id \
				<< idValue \
				<< columnName \
				<< columnValue \
				<< "createdAt" \
				<< mongo::Date_t(dt(QDateTime::now().to_string().c_str())));

		conn_->insert(collection_, p);
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return MONGO_ERR;
	}
	return MONGO_OK;
}

int32_t QMongoClient::insert(const char* id, const char* idValue, const char* columnName1, const char* columnValue1, const char* columnName2, const char* columnValue2)
{
	if(id == NULL || idValue == NULL || columnName1 == NULL || columnValue1 == NULL || columnName2 == NULL || columnValue2 == NULL)
		return MONGO_ERR;

	try {
		mongo::BSONObj p = BSON(GENOID \
				<< id \
				<< idValue \
				<< columnName1 \
				<< columnValue1 \
				<< columnName2 \
				<< columnValue2 \
				<< "createdAt" \
				<< mongo::Date_t(dt(QDateTime::now().to_string().c_str())));

		conn_->insert(collection_, p);
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return MONGO_ERR;
	}
	return MONGO_OK;
}

bool QMongoClient::select(const char* id, const char* idValue, const char* columnName, std::string& columnValue)
{
	if(id == NULL || idValue == NULL || columnName == NULL)
		return false;

	try {
		mongo::BSONObj res = conn_->findOne(collection_, BSON(id << idValue));
		if(!res.isEmpty()) {
			columnValue = res.getStringField(columnName);
			return true;
		}
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return false;
	}

	return false;
}

bool QMongoClient::select(const char* id, const char* idValue, const char* columnName1, std::string& columnValue1, const char* columnName2, std::string& columnValue2)
{
	if(id == NULL || idValue == NULL || columnName1 == NULL || columnName2 == NULL)
		return false;

	try {
		mongo::BSONObj res = conn_->findOne(collection_, BSON(id << idValue));
		if(!res.isEmpty()) {
			columnValue1 = res.getStringField(columnName1);
			columnValue2 = res.getStringField(columnName2);
			return true;
		}
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return false;
	}

	return false;
}

int32_t QMongoClient::update(const char* id, const char* idValue, const char* columnName, const char* columnValue)
{
	if(id == NULL || idValue == NULL || columnName == NULL || columnValue == NULL)
		return MONGO_ERR;

	try {
		mongo::BSONObj p = BSON(id \
				<< idValue);

		mongo::BSONObj after = BSON(id \
				<< idValue \
				<< columnName \
				<< columnValue \
				<< "createdAt" \
				<< mongo::Date_t(dt(QDateTime::now().to_string().c_str())));

		conn_->update(collection_, p, after, mongo::UpdateOption_Upsert);
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return MONGO_ERR;
	}
	return MONGO_OK;
}

int32_t QMongoClient::remove(const char* id, const char* idValue)
{
	try {
		conn_->remove(collection_, BSON(id << idValue));
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return MONGO_ERR;
	}
	return MONGO_OK;
}

bool QMongoClient::exists(const char* id, const char* idValue)
{
	try {
		std::auto_ptr<DBClientCursor> cursor =
			conn_->query(collection_, MONGO_QUERY(id << idValue));
		while(cursor->more()) {
			mongo::BSONObj p = cursor->next();
			return true;
		}
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return false;
	}

	return false;
}

unsigned long long QMongoClient::count()
{
	return conn_->count(collection_);
}

int32_t QMongoClient::dropCollection()
{
	try {
		conn_->dropCollection(collection_);
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return MONGO_ERR;
	}
	return MONGO_OK;
}

int32_t QMongoClient::createIndex(const char* columnName)
{
	try {
		conn_->createIndex(collection_, BSON(columnName << 1));
	} catch(const mongo::DBException& e) {
		Q_INFO("QMongoClient: database faild for (%s)...", e.toString().c_str());
		return MONGO_ERR;
	}
	return MONGO_OK;
}

unsigned long long QMongoClient::dt(const char* time)
{
	ptime epoch = time_from_string("1970-01-01 00:00:00.000");
	ptime now = time_from_string(time);
	time_duration diff = now - epoch;
	return diff.total_milliseconds();
}

Q_END_NAMESPACE

