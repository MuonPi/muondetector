#include "databaselink.h"

#include "log.h"

#include <type_traits>
#include <variant>

#include <curl/curl.h>

////////////////////////////////////////////////////////////////////////////////
/// ++++++++++++++++++
/// only for debugging
/// ++++++++++++++++++
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
struct data {
  char trace_ascii; /* 1 or 0 */
};

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;

  unsigned int width = 0x10;

  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
          text, (unsigned long)size, (unsigned long)size);

  for(i = 0; i<size; i += width) {

    fprintf(stream, "%4.4lx: ", (unsigned long)i);

    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i + c < size)
          fprintf(stream, "%02x ", ptr[i + c]);
        else
          fputs("   ", stream);
    }

    for(c = 0; (c < width) && (i + c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
         ptr[i + c + 1] == 0x0A) {
        i += (c + 2 - width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
         ptr[i + c + 2] == 0x0A) {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  struct data *config = (struct data *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */

  switch(type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
    /* FALLTHROUGH */
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// ------------------
/// only for debugging
/// ------------------
////////////////////////////////////////////////////////////////////////////////

namespace MuonPi {
DatabaseLink::Entry::Entry(const std::string& measurement, DatabaseLink& link)
    : m_link { link }
{
    m_stream<<measurement;
}

auto DatabaseLink::Entry::operator<<(const Influx::Tag& tag) -> Entry&
{
    m_stream<<','<<tag.name<<'='<<tag.field;
    return *this;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

auto DatabaseLink::Entry::operator<<(const Influx::Field& field) -> Entry&
{
    std::visit(overloaded {
                   [this, field](const std::string& value){m_stream<<(m_has_field?',':' ')<<field.name<<"=\""<<value<<'"';},
                   [this, field](std::int_fast64_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::size_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::uint8_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::uint16_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](std::uint32_t value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;},
                   [this, field](bool value){m_stream<<field.name<<(m_has_field?',':' ')<<'='<<value;},
                   [this, field](double value){m_stream<<(m_has_field?',':' ')<<field.name<<'='<<value;}
               }, field.value);
    return *this;
}

auto DatabaseLink::Entry::operator<<(std::int_fast64_t timestamp) -> bool
{
    m_stream<<' '<<timestamp;
    return m_link.send_string(m_stream.str());
}

DatabaseLink::DatabaseLink(const std::string& server, const LoginData& login, const std::string& database)
    : m_server { server }
    , m_login_data { login }
    , m_database { database }
{

}

DatabaseLink::~DatabaseLink() = default;

auto DatabaseLink::measurement(const std::string& measurement) -> Entry
{
    return Entry{measurement, *this};
}


auto DatabaseLink::send_string(const std::string& query) -> bool
{

    CURL *curl;
    struct data config;
    std::ostringstream url {};
    url
        <<m_server
        <<"/write?db="
        <<m_database
        <<"&u="<<m_login_data.username
        <<"&p="<<m_login_data.password
        <<"&epoch=ms";

    config.trace_ascii = 1; /* enable ascii tracing */

    curl = curl_easy_init();
    if(curl) {
        // +++ only for debugging
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        // --- only for debugging

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());

        CURLcode res { curl_easy_perform(curl) };

        long http_code { 0 };
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

        if((res != CURLE_OK)) {
            Log::warning()<<"Couldn't query Database: " + std::string{curl_easy_strerror(res)};
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return true;
}

} // namespace MuonPi
