# projetreseau
A scheduler to scan network vulnerabilities on a network.

## What does it do ?

On startup, it runs every command inside `src/scheduler/agents.conf` and returns the output. The thing is, commands are encrypted (at least they should), and the output is returned to the terminal. Bonus point : it runs on the network (see how `agents.conf` is organized) and on specific ports.

## Dependencies
A single depency other than stdlib : 
OpenSSL.


## How to compile it ?

To compile `scheduler.c` from root of project
```bash
gcc src/main.c -Wvla -Wall -Wextra -fsanitize=address,undefined src/scheduler/scheduler.c -o scheduler -lssl -lcrypto
```

To compile `agent.c`
```bash
gcc src/agent/agent.c -Wvla -Wall -Wextra -fsanitize=address,undefined -lssl -lcrypto -o agent
```

There is not really a makefile or CMake. I could implement them but don't feel the need for such a small project.

## How to run it ?

### Generate self-signed SSL certificates

Just follow the answer [here on StackOverflow](https://stackoverflow.com/questions/10175812/how-can-i-generate-a-self-signed-ssl-certificate-using-openssl).

Make sure your certificate is in `./security/cert.crt` and your key in `./security/key.key`

If you want to store it anywhere else, you'll need to change the paths in `src/agent/agent.c`, precisely [here](https://github.com/Esteban795/projetreseau/blob/main/src/agent/agent.c#L34) !


### Actually run 
To run the agents (for demonstration purposes) : 
```bash
./agent <port>
```

To run the scheduler : 
```bash
./scheduler <number_of_lines_in_agents.conf>
```

## Any issues ?
Open one on the repo !