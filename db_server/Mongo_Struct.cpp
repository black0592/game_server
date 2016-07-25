/*
 * Mongo_Struct.cpp
 *
 *  Created on: 2016年7月20日
 *      Author: zhangyalei
 */

#include "Mongo_Operator.h"
#include "Mongo_Struct.h"
#include "DB_Manager.h"

Mongo_Struct::Mongo_Struct(Xml &xml, TiXmlNode *node) : DB_Struct(xml, node) {
	std::stringstream stream;
	stream.str("game.");
	db_name_ = stream.str() + db_name_;
}

Mongo_Struct::~Mongo_Struct(){}

void Mongo_Struct::create_data(int64_t index){
	BSONObjBuilder set_builder ;
	for(std::vector<Field_Info>::iterator iter = field_vec_.begin();
			iter != field_vec_.end(); iter++){
		if((*iter).field_label == "arg"){
			create_data_arg(*iter, set_builder, index);
		}
		else if((*iter).field_label == "vector"){
			create_data_vector(*iter, set_builder);
		}
		else if((*iter).field_label == "struct"){
			create_data_struct(*iter, set_builder);
		}
	}
	MONGO_CONNECTION.update(db_name_, MONGO_QUERY(index_ << (long long int)index),
				BSON("$set" << set_builder.obj() ), true);
}

void Mongo_Struct::load_data(int64_t index, Block_Buffer &buffer){
	std::vector<BSONObj> total_record;
	if(index == 0){
		int total_count = MONGO_CONNECTION.count(db_name_);
		if(total_count > 0)
			MONGO_CONNECTION.findN(total_record, db_name_, Query(), total_count);
	}
	else {
		BSONObj obj = MONGO_CONNECTION.findOne(db_name_, MONGO_QUERY(index_ << (long long int)index));
		total_record.push_back(obj);
	}

	if(index == 0){
		buffer.write_uint16(total_record.size());
	}
	for (std::vector<BSONObj>::iterator iter = total_record.begin();
			iter != total_record.end(); ++iter) {
		for(std::vector<Field_Info>::iterator it = field_vec_.begin();
				it != field_vec_.end(); it++){
			if((*it).field_label == "arg"){
				build_buffer_arg(*it, buffer, *iter);
			}
			else if((*it).field_label == "vector"){
				build_buffer_vector(*it, buffer, *iter);
			}
			else if((*it).field_label == "struct"){
				build_buffer_struct(*it, buffer, *iter);
			}
		}
	}
}

void Mongo_Struct::save_data(Block_Buffer &buffer){
	BSONObjBuilder set_builder;
	int64_t index_value = buffer.peek_int64();
	for(std::vector<Field_Info>::iterator iter = field_vec_.begin();
			iter != field_vec_.end(); iter++){
		if((*iter).field_label == "arg"){
			build_bson_arg(*iter, buffer, set_builder);
		}
		else if((*iter).field_label == "vector"){
			build_bson_vector(*iter, buffer, set_builder);
		}
		else if((*iter).field_label == "struct"){
			build_bson_struct(*iter, buffer, set_builder);
		}
	}
	MONGO_CONNECTION.update(db_name_, MONGO_QUERY(index_ << (long long int)index_value),
			BSON("$set" << set_builder.obj() ), true);

	LOG_DEBUG("table %s save index:%ld", db_name_.c_str(), index_value);
}

void Mongo_Struct::save_data_vector(Block_Buffer &buffer){
	uint16_t count = buffer.read_uint16();
	LOG_DEBUG("table %s save size:%d", db_name_.c_str(), count);
	for(int i = 0; i < count; i++){
		save_data(buffer);
	}
}

void Mongo_Struct::delete_data(Block_Buffer &buffer) {
	uint16_t count = buffer.read_uint16();
	int64_t index = 0;
	for(int i = 0; i < count; i++){
		index = buffer.read_int64();
		MONGO_CONNECTION.remove(db_name_, MONGO_QUERY(index_ << (long long int)(index)));
	}
}

void Mongo_Struct::create_data_arg(Field_Info &field_info, BSONObjBuilder &builder, int64_t index){
	if(field_info.field_type == "int16"){
		int16_t value = 0;
		builder << field_info.field_name << (int)value;
	}
	else if(field_info.field_type == "int32"){
		int32_t value = 0;
		builder << field_info.field_name << (int)value;
	}
	else if(field_info.field_type == "int64"){
		int64_t value = 0;
		if(field_info.field_name == index_)
			value = index;
		builder << field_info.field_name << (long long int)value;
	}
	else if(field_info.field_type == "string"){
		std::string value = "";
		builder << field_info.field_name << value;
	}
	else if(field_info.field_type == "bool"){
		bool value = 0;
		builder << field_info.field_name << value;
	}
	else if(field_info.field_type == "double"){
		double value = 0;
		builder << field_info.field_name << value;
	}
	else {
		LOG_ERROR("Can not find the type %s", field_info.field_type.c_str());
	}
}

void Mongo_Struct::create_data_vector(Field_Info &field_info, BSONObjBuilder &builder){
	std::vector<BSONObj> builder_vec;
	builder << field_info.field_name << builder_vec;
}

void Mongo_Struct::create_data_struct(Field_Info &field_info, BSONObjBuilder &builder){
	DB_Manager::DB_Struct_Name_Map::iterator iter = DB_MANAGER->db_struct_name_map().find(field_info.field_type);
	if(iter == DB_MANAGER->db_struct_name_map().end()){
		LOG_ERROR("Can not find the module %s", field_info.field_type.c_str());
		return;
	}

	DB_Struct *db_struct  = iter->second;
	std::vector<Field_Info> field_vec = db_struct->field_vec();
	BSONObjBuilder obj_builder;
	for(std::vector<Field_Info>::iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++){
		if((*iter).field_label == "arg"){
			create_data_arg(*iter, obj_builder, 0);
		}
		else if((*iter).field_label == "vector"){
			create_data_vector(*iter, obj_builder);
		}
		else if((*iter).field_label == "struct"){
			create_data_struct(*iter, obj_builder);
		}
	}
	builder << field_info.field_type << obj_builder.obj();
}

void Mongo_Struct::build_bson_arg(Field_Info &field_info, Block_Buffer &buffer, BSONObjBuilder &builder){
	if(field_info.field_type == "int16"){
		int16_t value = buffer.read_int16();
		builder << field_info.field_name << (int)value;
	}
	else if(field_info.field_type == "int32"){
		int32_t value = buffer.read_int32();
		builder << field_info.field_name << (int)value;
	}
	else if(field_info.field_type == "int64"){
		int64_t value = buffer.read_int64();
		builder << field_info.field_name << (long long int)value;
	}
	else if(field_info.field_type == "string"){
		std::string value = buffer.read_string();
		builder << field_info.field_name << value;
	}
	else if(field_info.field_type == "bool"){
		bool value = buffer.read_bool();
		builder << field_info.field_name << value;
	}
	else if(field_info.field_type == "double"){
		double value = buffer.read_double();
		builder << field_info.field_name << value;
	}
	else {
		LOG_ERROR("Can not find the type %s", field_info.field_type.c_str());
	}
}

void Mongo_Struct::build_bson_vector(Field_Info &field_info, Block_Buffer &buffer, BSONObjBuilder &builder){
	std::vector<BSONObj> bson_vec;
	uint16_t vec_size = buffer.read_uint16();

	if(is_struct(field_info.field_type)){
		for(uint16_t i = 0; i < vec_size; ++i) {
			BSONObjBuilder obj_builder;
			build_bson_struct(field_info, buffer, obj_builder);
			bson_vec.push_back(obj_builder.obj());
		}
	}
	else{
		for(uint16_t i = 0; i < vec_size; ++i) {
			BSONObjBuilder obj_builder;
			build_bson_arg(field_info, buffer, obj_builder);
			bson_vec.push_back(obj_builder.obj());
		}
	}

	builder << field_info.field_name << bson_vec;
}

void Mongo_Struct::build_bson_struct(Field_Info &field_info, Block_Buffer &buffer, BSONObjBuilder &builder){
	DB_Manager::DB_Struct_Name_Map::iterator iter = DB_MANAGER->db_struct_name_map().find(field_info.field_type);
	if(iter == DB_MANAGER->db_struct_name_map().end()){
		LOG_ERROR("Can not find the module %s", field_info.field_type.c_str());
		return;
	}

	DB_Struct *db_struct  = iter->second;
	std::vector<Field_Info> field_vec = db_struct->field_vec();
	BSONObjBuilder obj_builder;
	for(std::vector<Field_Info>::iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++){
		if((*iter).field_label == "arg"){
			build_bson_arg(*iter, buffer, obj_builder);
		}
		else if((*iter).field_label == "vector"){
			build_bson_vector(*iter, buffer, obj_builder);
		}
		else if((*iter).field_label == "struct"){
			build_bson_struct(*iter, buffer, obj_builder);
		}
	}
	builder << field_info.field_type << obj_builder.obj();
}

void Mongo_Struct::build_buffer_arg(Field_Info &field_info, Block_Buffer &buffer, BSONObj &bsonobj) {
	if(field_info.field_type == "int16"){
		int16_t value = bsonobj[field_info.field_name].numberInt();
		buffer.write_int16(value);
	}
	else if(field_info.field_type == "int32"){
		int32_t value = bsonobj[field_info.field_name].numberInt();
		buffer.write_int32(value);
	}
	else if(field_info.field_type == "int64"){
		int64_t value = bsonobj[field_info.field_name].numberLong();
		buffer.write_int64(value);
	}
	else if(field_info.field_type == "string"){
		std::string value = bsonobj[field_info.field_name].valuestrsafe();
		buffer.write_string(value);
	}
	else if(field_info.field_type == "bool"){
		bool value = bsonobj[field_info.field_name].numberInt();
		buffer.write_bool(value);
	}
	else if(field_info.field_type == "double"){
		double value = bsonobj[field_info.field_name].numberDouble();
		buffer.write_double(value);
	}
	else {
		LOG_ERROR("Can not find the type %s", field_info.field_type.c_str());
	}
}

void Mongo_Struct::build_buffer_vector(Field_Info &field_info, Block_Buffer &buffer, BSONObj &bsonobj) {
	BSONObj fieldobj = bsonobj.getObjectField(field_info.field_name);
	uint16_t count = fieldobj.nFields();
	buffer.write_uint16(count);

	BSONObjIterator field_iter(fieldobj);
	BSONObj obj;
	if(is_struct(field_info.field_type)){
		while (field_iter.more()) {
			obj = field_iter.next().embeddedObject();
			build_buffer_struct(field_info, buffer, obj);
		}
	}
	else{
		while (field_iter.more()) {
			obj = field_iter.next().embeddedObject();
			build_buffer_arg(field_info, buffer, obj);
		}
	}
}

void Mongo_Struct::build_buffer_struct(Field_Info &field_info, Block_Buffer &buffer, BSONObj &bsonobj) {
	DB_Manager::DB_Struct_Name_Map::iterator iter = DB_MANAGER->db_struct_name_map().find(field_info.field_type);
	if(iter == DB_MANAGER->db_struct_name_map().end()){
		LOG_ERROR("Can not find the module %s", field_info.field_type.c_str());
		return;
	}

	DB_Struct *db_struct  = iter->second;
	BSONObj fieldobj = bsonobj.getObjectField(field_info.field_type);
	std::vector<Field_Info> field_vec = db_struct->field_vec();
	for(std::vector<Field_Info>::iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++){
		if((*iter).field_label == "arg"){
			build_buffer_arg(*iter, buffer, fieldobj);
		}
		else if((*iter).field_label == "vector"){
			build_buffer_vector(*iter, buffer, fieldobj);
		}
		else if((*iter).field_label == "struct"){
			build_buffer_struct(*iter, buffer, fieldobj);
		}
	}
}
