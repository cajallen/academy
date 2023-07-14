#include "editor/editor.hpp"

int main() {
	spellbook::editor.startup();
	spellbook::editor.run();
	spellbook::editor.shutdown();

	return 0;
}
