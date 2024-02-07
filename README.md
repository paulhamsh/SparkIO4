# SparkIO4
## Revised SparkIO for GO and MINI compatibility

## Key changes

The Spark GO and MINI proved less reliable in BLE communications with the ESP32 especially when retrieving a preset.   
This caused issues for SparkMINI and SparkBox.   

The previous library used a streaming approach, removing and decoding the Spark format as the data was streamed.   
This caused issues when data was incomplete.   

The new library will read and process packets.   


## Packet determination
The Spark amps cuts packets up into chunks with specific maximum lengths. For Spark 40 this is 106 bytes, and for the GO and MINI, this is 90 bytes cut further into 4 chunks of 20 and a chunk of 10.    
This means a full packet will comprise multipes of these chunk sizes, with a possible final smaller chunk.   
The packet will always end in a byte of 0xf7 (see the specification here https://github.com/paulhamsh/Spark/blob/main/Spark%20Protocol%20Description%20v3.2.pdf).   

A packet with this shorter final chunk is easy to detect because it will have a length which is not 10, 20 or 106.   
But if the final chunk is exactly one of these sizes (for example, the GO hardware preset change message) then it cannot be idenfitied as different from an intermediate chunk in the message that just happens to end in 0xf7. 

The approach taken here is to incorporate timeouts.   
If a message times out (takes more than a determined time to reach a shorter final chunk), then it is checked to see if the last byte was 0xf7.   
If so, then it is assumed to be a full message.   
If not, the message is assumed to be missing a final chunk (or more) and is dropped.

There is a risk that the final chunk received meets the above criteria when it is, in fact, just the last chunk before some that are missing. This seems low risk, and the next stage of decoding will have to check for this.   
