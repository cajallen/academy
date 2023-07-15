#include "client.hpp"

int main() {
	spellbook::get_client().startup();
	spellbook::get_client().run();
	spellbook::get_client().shutdown();

	return 0;
}
