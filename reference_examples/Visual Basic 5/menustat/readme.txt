Selected menu item demo

	This demo project shows how to use subclassing to return the currently selected menu item. It subclasses the window. If the message is about the menu, then it is intercepted, otherwise, it is passed to the normal message handler. The position of the menu item on the menu is retrived, then the caption of the item is retrieved, both using the windows API.

	This can be applied to any window, but be careful: any mistakes, and you are likely to crash visual basic, maybe even the whole system. Be careful!
