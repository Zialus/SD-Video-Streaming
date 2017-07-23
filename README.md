# VideoStreaming

Ice RPC and IceStorm are an integral part of this application and therefore before 
running any applications we need to start the icebox _runtime_ with the following command.

``` bash
icebox --Ice.Config=config.icebox
```

## Portal

The Portal needs to be running before trying launch any server or client.

It should be run by passing as an argument the ice config file.

``` bash
./executables/portal --Ice.Config="ice_config_dir/config.portal"
```

## Server

In order to see the necessary and/or optional arguments for the server, just pass the "--help" argument.

``` bash
./executables/server --help
```

## Client

The client let's the user list the available streams, pick a stream to be played, and get notifications about new streams.

It should be run by passing as an argument the ice config file.

``` bash
./executables/client --Ice.Config="ice_config_dir/config.client"
```

To check the available commands, just press `TAB`.
