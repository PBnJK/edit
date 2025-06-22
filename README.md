# edit

A simple text editor in C with ncurses

## Features

- [x] Basic Editing
- [x] Saving
- [x] Loading
- [x] More refined editing
- [x] Scrolling
- [x] Undo/Redo
- [ ] Copy & Paste (Kill & Yank)

## Dependencies

Just `ncurses`!

## Building

This project uses CMake for building. You can find many guides on how to use it
on the internet, but here's the "resumo da ópera." I'm assuming you have CMake
installed:

```sh
# Clone and enter the repo
git clone https://github.com/pbnjk/edit.git && cd edit

# Create and enter the build directory
mkdir build && cd build

# Set up the build
cmake ..

# You can add some extra options on the last step
# I set it up like this usually:
# cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Make the project!
make
```

It may complain about `ncurses` being missing. Again, I trust you to be able to
install it on your own...

## Other Things

This project was created for the [Summer of Making](https://summer.hackclub.com)
event. Here's to that Framework laptop!

The [ncurses](https://invisible-island.net/ncurses/) library is licensed under
the MIT license.

This project is licensed under the MIT license. You may use it for whatever,
though I don't recommend you do. Everything you can learn here can be found
implemented better and more efficiently in other projects and repositories.

At the very least, I hope this encourages you to create similarly useless
software. It may not seem like it, but it is one of the many joys this world
still has to offer.

This project was made without the help of LLMs. I think that is worth something
these days.

You can open a PR if you like. Please make it clear if you used an LLM to write
it; I'll still look it over if you do...

Thanks! Love you! Kisses!
