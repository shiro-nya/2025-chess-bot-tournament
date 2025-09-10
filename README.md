# 2025-chess-bot-tournament
A challenge to write a tiny chess bot in one month! The challenge is open to all levels of skill, and an effort has been made to make it accessible to beginners. After the entry period is over, submitted bots will compete in a tournament to see which is strongest! 

## Disclaimer: I don't really know what I'm doing
This is my first time hosting something like this, it's almost a certainty that I've forgotten some essential detail or that something needs clarification. I am happy to answer any questions regarding the format, rules, or give details on the development environment if needed. I will try to keep an up-to-date FAQ, as well as language-specific FAQs if necessary.

If you have questions, or requests for language support, feel free to reach out to me on Discord: @shiro_nya

## Contest Format
### Schedule
You may submit at any time prior to submissions closing (how to submit explained in the next section). Submissions will be accepted until November 1st @ 12:00AM EST. Following this, received valid submissions will be entered into a tournament to determine the final chess bot rankings.
### Tournament
The tournament will be a round-robin tournament in which each bot faces each other bot a number of times. Bot players will have 5 minutes of time, no increment. Pre-moved openings may be used to avoid bots repeatedly playing the exact same games.
### Results
Once the tournament is complete, the results will be released.

## Submissions
Please read this section carefully! It outlines what is available to your bot, and how to make a valid submission.
### Chess API
A basic Chess API will be provided for supported languages, which acts as an interface to the chess game. This API will be readily accessible by your code. Some of its features are:

- Primitives to represent Boards and Moves
- Methods to get your remaining time
- Methods for submitting moves

The detailed docs can be found [here](API_LINK_HERE).
### Language Support
The Chess API currently exists for the following languages:

- C
- Python 3.12

I am partly willing to support more languages if they are easy to incorporate. Feel free reach out to me if you have a language you would like to request, but I won't make any guarantees.

I welcome crowdsourced implementations of the Chess API for various languages, so long as they don't introduce anything more or less than the functionality specified in the [docs](API_LINK_HERE). If you are willing to share these implementations publicly, I will happily make them official (and thus not counted toward the token limit).
### Submission Format
Your submission should be a single source code file for your language of choice. Near the top of the file should be a comment containing the following:

- Author(s)
- Bot name
- *(Optional)* A description for your bot
### Code Guidelines
All code submissions must follow the below rules:

- **Do not make any attempts to damage, disrupt or otherwise sadden my system, escape the virtual environment, or interfere with the tournament backend.**
- Generally, the standard library for your language is allowed. However, **your bot must not make any network connections, read or write files, or use built-in arbitrary execution functions** (e.g. Python's `exec()`). Sneaky ways of getting around this rule on a technicality will get you DQ'd. Don't do it. Thanks.
- **Non-standard libraries and modules will not be accommodated,** outside of special exceptions (e.g. a library to add bindings for a Chess API implementation). Anything you wish to use from such libraries must be included in your source file, and will count toward the token limit.
- **Your source code submission must not exceed 1024 tokens.** This refers to the tokens the lexer for your language outputs. For example, `int myvar = 4;` may become the tokens `["int", "myvar", "=", "4", ";"]`. Tokens for a language will be evaluated in the same way as the out-of-the-box lexer for that language does. More details on how to calculate the token count of your source file can be found with the resources for your particular language.
- **Bots should not expect to start from a new chess board state,** and should always play from the board provided by the API. Tournament rounds may pre-play an opening move series to avoid deterministic outcomes.

I reserve the right to remove any bots from the competition at my own discretion, but of course I don't intend to exercise this unless necessary.
### Target System
The bots will be running in a virtual machine running 64-bit Lubuntu. You should expect to have 4GB of memory available for your bot. The VM host system has a Ryzen 7 5700X CPU, though I only supply two cores to the virtual machine. Not really sure how much this informs the performance, if you'd like any more details feel free to ask.

## Resources
These are a handful of resources I thought might be helpful, in no particular order:

- Chess API documentation:
- Chess programming wiki, for more detailed info on chess bot techniques: https://www.chessprogramming.org/Main_Page
- YouTube channel of Sebastian Lague, a large inspiration for putting this together: https://www.youtube.com/@SebastianLague
