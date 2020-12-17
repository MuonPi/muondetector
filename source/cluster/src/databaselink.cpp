#include "databaselink.h"


namespace MuonPi {

DatabaseLink::DatabaseLink(const std::string& server, const LoginData& login, const std::string& database)
:   m_server { server }
    , m_login_data { login }
    , m_database { database }
{
	m_server_info = influxdb_cpp::server_info(m_server, 8086, m_database, m_login_data.username, m_login_data.password);
}

DatabaseLink::~DatabaseLink() {

}


auto DatabaseLink::write_entry(const DbEntry& entry) -> bool
{
	std::scoped_lock<std::mutex> lock { m_mutex };
	influxdb_cpp::builder builder{};
	influxdb_cpp::detail::tag_caller& tags = builder.meas(entry.measurement());
	
	if (entry.tags().size()) {
		for (auto it=entry.tags().begin(); it!=entry.tags().end(); ++it) {
			tags.tag(it->first,it->second);
		}
	}
	
	influxdb_cpp::detail::field_caller& fields = tags.field("","");
	if (entry.fields().size()) {
		for (auto it=entry.fields().begin(); it!=entry.fields().end(); ++it) {
			fields.field(it->first,it->second);
		}
	}
	

	influxdb_cpp::detail::ts_caller& ts=fields.timestamp(stoull(entry.timestamp(), nullptr));
	ts.post_http(m_server_info);
	if (entry.measurement()=="doof") return 1;
	return 0;
}

} // namespace MuonPi