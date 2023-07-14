#include "editor/editor.hpp"

int main() {
	spellbook::get_editor().startup();
	spellbook::get_editor().run();
	spellbook::get_editor().shutdown();

	return 0;
}
