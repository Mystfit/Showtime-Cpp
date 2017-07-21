#pragma once

class ZstEntityBase {
public:
	ZstEntityBase(int id, const char * entity_type, const char * entity_name);
	const char * entity_type() const;
	const char * name() const;
	const int id() const;
private:
	char * m_entity_type;
	char * m_name;
	int m_id;
};