WorkerScript.onMessage = function(msg)
{
    try
    {
        if(msg.dump) print('>>>', msg.text)
        WorkerScript.sendMessage({ result: JSON.parse(msg.text) })
    }
    catch(e)
    {
        WorkerScript.sendMessage({ error: e.message })
    }
}
