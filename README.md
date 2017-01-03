# VideoStreaming

Ice RPC and IceStorm are an integral part of this application and therefore before 
running any applications we need to start the icebox _runtime_ with the following command.

``` bash
icebox --Ice.Config=config.icebox &

```

## Portal

The Portal needs to be running before trying launch any server or client.

It should be run without any arguments.

``` bash
./executables/portal
```

## Server

In order to see the necessary and/or optional arguments for the server, just pass the "--help" argument.

``` bash
./executables/server --help
```

## Client

The client should be run without any arguments.

``` bash
./executables/client
```