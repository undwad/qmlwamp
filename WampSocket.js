WorkerScript.onMessage = function(msg)
{
    if(msg.dump) print('>>>', msg.text)
    WorkerScript.sendMessage(JSON.parse(msg.text))
}
