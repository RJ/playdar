This is a small httpd lib written by Erik Frey (http://fawx.com/)

Currently it requires the entire response be loaded into memory before sending
to the client, which makes it less than ideal for use in Playdar for streaming.
This will be addressed at some point, but does the job for the time being.
Ideally we could send headers, then stream audio data to the
client as we receive it (from remote machine or local disk).
