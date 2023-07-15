#include "server.hpp"

int main() {
	spellbook::get_server().startup();
	spellbook::get_server().run();
	spellbook::get_server().shutdown();

	return 0;
}
