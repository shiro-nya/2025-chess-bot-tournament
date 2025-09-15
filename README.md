# 2025-chess-bot-tournament
Welcome to the tournament! This is a fun contest to try writing a small bot to play Chess! Once submitted, all the bots will face off in a tournament to decide the strongest!

If you have questions, or requests for language support, feel free to reach out to me on Discord: @shiro_nya

## Change Log
- If something changes during the contest (bug fixed in API, rules clarification), I will make a note of it here.

## How to participate
- Install a development environment for one of the supported languages: Python, C, C++ (the C API should be compatible)
- Get the resources for your language from its folder under `src`. Don't forget to check the language-specific README and the [docs](https://github.com/shiro-nya/2025-chess-bot-tournament/wiki)!
- Write some code!
- Run the program to test your changes, and measure its performance.
  - You will likely want a program that can interface with UCI to test your both with. One such program is [Arena](http://www.playwitharena.de), which will be used to host the tournament.
- When you're happy with your bot, submit it using the [submission form](https://forms.gle/hUD66B5Aec3Z64aJ7)!
  - You will be able to edit your entry until submissions close.

## Rules
Please read this section carefully! It outlines what is available to your bot, and how to make a valid submission.
- You may work with a group.
- Don't try to store code in variables or function/class/file metadata to be executed via an existing language feature. Writing your own VM is okay though, if you really want to go that route.
- The only libraries available will be the standard libraries for your language plus the chess API supplied in this repo. However, your bot must not make any network connections, read or write files, start or end threads or otherwise run code in the background/in parallel, or use built-in arbitrary execution functions (e.g. Python's `exec()`).
- Your source code submission must not exceed 1024 tokens. This refers to the tokens the lexer for your language outputs. These will be counted using [tcount](https://github.com/RRethy/tcount/tree/master).
- Bots shouldn't expect to start from a new chess board state, and should always play from the board provided by the API. Tournament rounds may pre-play an opening move series.

## FAQ
- Can you add support for my language?
  - I'll consider adding support for languages granted the token counter supports them. I welcome community submissions of language interfaces to the provided API as long as they don't provide additional functionality, if you end up making and sharing one then I'll let people use it for free (i.e. it won't count toward your token limit).
