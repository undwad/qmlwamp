WorkerScript.onMessage = function(msg)
{
    try
    {
        if(msg.dump) print('>>>', msg.text)
        msg.result = JSON.parse(msg.text)
    }
    catch(e)
    {
        msg.exception = e
    }
    msg.text = null
    WorkerScript.sendMessage(msg)
}
