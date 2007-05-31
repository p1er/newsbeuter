/* test driver for newsbeuter */

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#include <unistd.h>

#include <cache.h>
#include <rss.h>
#include <configcontainer.h>
#include <keymap.h>
#include <xmlpullparser.h>
#include <urlreader.h>
#include <utils.h>
#include <matcher.h>

#include <stdlib.h>

using namespace newsbeuter;

BOOST_AUTO_TEST_CASE(TestNewsbeuterReload) {
	configcontainer * cfg = new configcontainer();
	cache * rsscache = new cache("test-cache.db", cfg);

	rss_parser parser("http://bereshit.synflood.at/~ak/rss.xml", rsscache, cfg);
	rss_feed feed = parser.parse();
	BOOST_CHECK_EQUAL(feed.items().size(), 8u);

	rsscache->externalize_rssfeed(feed);
	rsscache->internalize_rssfeed(feed);
	BOOST_CHECK_EQUAL(feed.items().size(), 8u);

	BOOST_CHECK_EQUAL(feed.items()[0].title(), "Teh Saxxi");
	BOOST_CHECK_EQUAL(feed.items()[7].title(), "Handy als IR-Detektor");

	feed.items()[0].set_title("Another Title");
	feed.items()[0].set_pubDate(time(NULL));
	BOOST_CHECK_EQUAL(feed.items()[0].title(), "Another Title");

	rsscache->externalize_rssfeed(feed);

	rss_feed feed2(rsscache);
	feed2.set_rssurl("http://bereshit.synflood.at/~ak/rss.xml");
	rsscache->internalize_rssfeed(feed2);

	BOOST_CHECK_EQUAL(feed2.items().size(), 8u);
	BOOST_CHECK_EQUAL(feed2.items()[0].title(), "Another Title");
	BOOST_CHECK_EQUAL(feed2.items()[7].title(), "Handy als IR-Detektor");

	delete rsscache;
	delete cfg;

	::unlink("test-cache.db");
}

BOOST_AUTO_TEST_CASE(TestConfigParserAndContainer) {
	configcontainer * cfg = new configcontainer();
	configparser cfgparser;
	cfg->register_commands(cfgparser);

	try {
		cfgparser.parse("test-config.txt");
	} catch (...) {
		BOOST_CHECK(false);
	}

	// test boolean config values
	BOOST_CHECK_EQUAL(cfg->get_configvalue("show-read-feeds"), "no");
	BOOST_CHECK_EQUAL(cfg->get_configvalue_as_bool("show-read-feeds"), false);

	// test string config values
	BOOST_CHECK_EQUAL(cfg->get_configvalue("browser"), "firefox");

	// test integer config values
	BOOST_CHECK_EQUAL(cfg->get_configvalue("max-items"), "100");
	BOOST_CHECK_EQUAL(cfg->get_configvalue_as_int("max-items"), 100);

	// test ~/ expansion for path config values
	std::string cachefilecomp = ::getenv("HOME");
	cachefilecomp.append("/foo");
	BOOST_CHECK(cfg->get_configvalue("cache-file") == cachefilecomp);

	delete cfg;
}

BOOST_AUTO_TEST_CASE(TestKeymap) {
	keymap k;
	BOOST_CHECK_EQUAL(k.get_operation("ENTER"), OP_OPEN);
	BOOST_CHECK_EQUAL(k.get_operation("CHAR(117)"), OP_SHOWURLS);
	BOOST_CHECK_EQUAL(k.get_operation("CHAR(88)"), OP_NIL);

	k.unset_key("enter");
	BOOST_CHECK_EQUAL(k.get_operation("ENTER"), OP_NIL);
	k.set_key(OP_OPEN, "enter");
	BOOST_CHECK_EQUAL(k.get_operation("ENTER"), OP_OPEN);

	BOOST_CHECK_EQUAL(k.get_opcode("open"), OP_OPEN);
	BOOST_CHECK_EQUAL(k.get_opcode("some-noexistent-operation"), OP_NIL);

	BOOST_CHECK_EQUAL(k.getkey(OP_OPEN), "enter");
	BOOST_CHECK_EQUAL(k.getkey(OP_TOGGLEITEMREAD), "N");
	BOOST_CHECK_EQUAL(k.getkey(static_cast<operation>(30000)), "<none>");

	BOOST_CHECK_EQUAL(k.get_key("CHAR(32)"), ' ');
	BOOST_CHECK_EQUAL(k.get_key("CHAR(85)"), 'U');
	BOOST_CHECK_EQUAL(k.get_key("CHAR(126)"), '~');
	BOOST_CHECK_EQUAL(k.get_key("INVALID"), 0);
}

BOOST_AUTO_TEST_CASE(TestXmlPullParser) {
	std::istringstream is("<test><foo quux='asdf' bar=\"qqq\">text</foo>more text<more>&quot;&#33;&#x40;</more></test>");
	xmlpullparser xpp;
	xmlpullparser::event e;
	xpp.setInput(is);

	e = xpp.getEventType();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_DOCUMENT);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "test");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), 0);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "foo");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), 2);
	BOOST_CHECK_EQUAL(xpp.getAttributeValue("quux"), "asdf");
	BOOST_CHECK_EQUAL(xpp.getAttributeValue("bar"), "qqq");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "text");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), -1);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "foo");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), -1);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "more text");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "more");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "\"!@");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "more");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "test");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_DOCUMENT);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_DOCUMENT);
}

BOOST_AUTO_TEST_CASE(TestUrlReader) {
	urlreader u;
	u.load_config("test-urls.txt");
	BOOST_CHECK_EQUAL(u.get_urls().size(), 3u);
	BOOST_CHECK_EQUAL(u.get_urls()[0], "http://test1.url.cc/feed.xml");
	BOOST_CHECK_EQUAL(u.get_urls()[1], "http://anotherfeed.com/");
	BOOST_CHECK_EQUAL(u.get_urls()[2], "http://onemorefeed.at/feed/");

	BOOST_CHECK_EQUAL(u.get_tags("http://test1.url.cc/feed.xml").size(), 2u);
	BOOST_CHECK_EQUAL(u.get_tags("http://test1.url.cc/feed.xml")[0], "tag1");
	BOOST_CHECK_EQUAL(u.get_tags("http://test1.url.cc/feed.xml")[1], "tag2");
	BOOST_CHECK_EQUAL(u.get_tags("http://anotherfeed.com/").size(), 0u);
	BOOST_CHECK_EQUAL(u.get_tags("http://onemorefeed.at/feed/").size(), 2u);

	BOOST_CHECK_EQUAL(u.get_alltags().size(), 3u);
}

BOOST_AUTO_TEST_CASE(TestTokenizers) {
	std::vector<std::string> tokens;

	tokens = utils::tokenize("as df qqq");
	BOOST_CHECK_EQUAL(tokens.size(), 3u);
	BOOST_CHECK(tokens[0] == "as" && tokens[1] == "df" && tokens[2] == "qqq");

	tokens = utils::tokenize(" aa ");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "aa");

	tokens = utils::tokenize("	");
	BOOST_CHECK_EQUAL(tokens.size(), 0u);
	
	tokens = utils::tokenize("");
	BOOST_CHECK_EQUAL(tokens.size(), 0u);

	tokens = utils::tokenize_spaced("a b");
	BOOST_CHECK_EQUAL(tokens.size(), 3u);
	BOOST_CHECK_EQUAL(tokens[1], " ");

	tokens = utils::tokenize_spaced(" a\t b ");
	BOOST_CHECK_EQUAL(tokens.size(), 5u);
	BOOST_CHECK_EQUAL(tokens[0], " ");
	BOOST_CHECK_EQUAL(tokens[1], "a");
	BOOST_CHECK_EQUAL(tokens[2], " ");

	tokens = utils::tokenize_quoted("asdf \"foobar bla\" \"foo\\r\\n\\tbar\"");
	BOOST_CHECK_EQUAL(tokens.size(), 3u);
	BOOST_CHECK_EQUAL(tokens[0], "asdf");
	BOOST_CHECK_EQUAL(tokens[1], "foobar bla");
	BOOST_CHECK_EQUAL(tokens[2], "foo\r\n\tbar");

	tokens = utils::tokenize_quoted("  \"foo \\\\xxx\"\t\r \" \"");
	BOOST_CHECK_EQUAL(tokens.size(), 2u);
	BOOST_CHECK_EQUAL(tokens[0], "foo \\xxx");
	BOOST_CHECK_EQUAL(tokens[1], " ");

	tokens = utils::tokenize_quoted("\"\\\\");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\");

	// the following test cases specifically demonstrate a problem of the tokenize_quoted with several \\ sequences directly appended
	tokens = utils::tokenize_quoted("\"\\\\\\\\");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\\\\\");
}

struct testmatchable : public matchable {
	virtual bool has_attribute(const std::string& attribname) {
		if (attribname == "abcd" || attribname == "AAAA")
			return true;
		return false;
	}

	virtual std::string get_attribute(const std::string& attribname) {
		if (attribname == "abcd")
			return "xyz";
		if (attribname == "AAAA")
			return "12345";
		return "";
	}
};

BOOST_AUTO_TEST_CASE(TestFilterLanguage) {
	FilterParser fp;

	// test parser
	BOOST_CHECK_EQUAL(fp.parse_string("a = \"b\""), true);
	BOOST_CHECK_EQUAL(fp.parse_string("a = \"b"), false);
	BOOST_CHECK_EQUAL(fp.parse_string("a = b"), false);
	BOOST_CHECK_EQUAL(fp.parse_string("(a=\"b\")"), true);
	BOOST_CHECK_EQUAL(fp.parse_string("((a=\"b\"))"), true);
	BOOST_CHECK_EQUAL(fp.parse_string("((a=\"b\")))"), false);

	// test operators
	BOOST_CHECK_EQUAL(fp.parse_string("a != \"b\""), true);
	BOOST_CHECK_EQUAL(fp.parse_string("a =~ \"b\""), true);
	BOOST_CHECK_EQUAL(fp.parse_string("a !~ \"b\""), true);
	BOOST_CHECK_EQUAL(fp.parse_string("a !! \"b\""), false);

	// complex query
	BOOST_CHECK_EQUAL(fp.parse_string("( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\""), true);

	testmatchable tm;
	matcher m;

	m.parse("abcd = \"xyz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("abcd = \"uiop\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("abcd != \"uiop\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("abcd != \"xyz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);

	// TODO: use "AAAA" attribute to test regex matching as soon as it gets implemented

}