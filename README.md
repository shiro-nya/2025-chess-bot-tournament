# 2025-chess-bot-tournament
![Tournament banner](/resources/chess_tournament_banner.png)

Welcome to the tournament! This is a fun contest to try writing a small bot to play Chess! Once submitted, all the bots will face off in a tournament to decide the strongest!

If you have questions, or requests for language support, feel free to reach out to me on Discord: @shiro_nya

## Change Log
- 2025/09/19: Added two functions to the Chess API for fetching board turn counts, and one for cloning boards.
- 2025/09/18: Added a function to the Chess API for fetching the opponent's latest move.
- 2025/09/17: Six functions have been added to the Chess API to make Bitboards slightly easier to use.

## How to participate
- Install a development environment for one of the supported languages: Python, C, C++ (the C API should be compatible), Rust
- Get the resources for your language from its folder under `src`. Don't forget to check the language-specific README and the [docs](https://github.com/shiro-nya/2025-chess-bot-tournament/wiki)!
- Write some code!
- Run the program to test your changes, and measure its performance.
  - You will likely want a program that can interface with UCI to test your bot with. One such program is [Cute Chess](https://cutechess.com), which will be used to host the tournament.
- When you're happy with your bot, submit your source code using the [submission form](https://forms.gle/hUD66B5Aec3Z64aJ7)!
  - You will be able to edit your entry until submissions close.
  - Submissions will close **November 1, 2025 @ 12:00AM EST**

## Rules
Please read this section carefully! It outlines what is available to your bot, and how to make a valid submission.
- You may work alone or with a group.
- Don't try to store code in variables or function/class/file metadata to be executed via an existing language feature. In particular, don't try to hide code in arbitrary-length strings or ints as pertains to your language. Writing your own VM is okay though, if you really want to go that route.
- The only libraries available will be the standard libraries for your language plus the chess API supplied in this repo. However, your bot must not make any network connections, read or write files, start or end threads or otherwise run code in the background/in parallel, or use built-in arbitrary execution functions (e.g. Python's `exec()`).
- Your source code submission must not exceed 1024 tokens. This refers to the tokens the lexer for your language outputs. These will be counted using [toknt](https://github.com/chayleaf/toknt/tree/master).
- Bots shouldn't expect to start from a new chess board state, and should always play from the board provided by the API. Tournament rounds may pre-play an opening move series. Bots may not be restarted between tournament games, so be careful what you cache.

## FAQ
- Can you add support for my language?
  - I'll consider adding support for languages granted the token counter supports them. I welcome community submissions of language interfaces to the provided API as long as they don't provide additional functionality, if you end up making and sharing one then I'll let people use it for free (i.e. it won't count toward your token limit).

## Credits
- Sebastian Lague, whose [video](https://www.youtube.com/watch?v=Ne40a5LkK6A) inspired this contest!
- Everyone who has kindly contributed bindings to the Chess API
- [chayleaf](https://github.com/chayleaf), who made a vastly improved token counter and very much made this contest possible
