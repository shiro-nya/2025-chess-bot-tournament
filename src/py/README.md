# Getting started (Python language version)

## Setup
- Ensure you have a working Python version installed
  - The API was tested with Python 3.12.11, so this is the recommended version.
- Copy all of the `.py` files into a directory
- Copy all of the bitboard and chessapi `.c`, `.h` files from the C section of this Git repo into the same directory
- Install `cffi`, `setuptools` using `pip install`
  - It's a good idea to do this in a Python virtual environment!
- Run `build_chessapi_ffi.py`. This will create the Python module that hosts the API. You only have to do this once.
- Modify `example_bot.py` with your own code
- Run your code!
  - Tip: Easiest to test by adding as a bot to a compatible chess program

Be sure to refer to the docs to see how to use the API!
