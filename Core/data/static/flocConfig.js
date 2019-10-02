var webSocket = null;

$(function()
{
    connect();

});

function setid()
{
    if (!$('#set_tid').val() || !$('#set_uid_h').val() || !$('#set_uid_l').val())
    {
        alert("未記入のIDがあります。");
        return;
    }

    const tid = parseInt($('#set_tid').val(), 16);
    const uid_h = parseInt($('#set_uid_h').val(), 16);
    const uid_l = parseInt($('#set_uid_l').val(), 16);

    const tid_ok = (tid >= 0) && (tid <= 255);
    const uid_h_ok = (uid_h >= 0) && (uid_h <= 255);
    const uid_l_ok = (uid_l >= 0) && (uid_l <= 255);

    if (tid_ok && uid_h_ok && uid_l_ok)
    {
        console.log("setid "
            + tid.toString(16).toUpperCase() + " "
            + uid_h.toString(16).toUpperCase() + " "
            + uid_l.toString(16).toUpperCase()
        );

        send("setid "
            + tid.toString(16).toUpperCase() + " "
            + uid_h.toString(16).toUpperCase() + " "
            + uid_l.toString(16).toUpperCase()
        );
    } else
    {
        alert("記入されたIDの中に不適切なものがあります。");
        return;
    }

}

function setled()
{
    if (!$('#led_uid_h').val() || !$('#led_uid_l').val())
    {
        alert("未記入のIDがあります。");
        return;
    }
    if (!$('#led_value').val())
    {
        alert("Valueが未記入です。");
        return;
    }

    const uid_h = parseInt($('#led_uid_h').val(), 16);
    const uid_l = parseInt($('#led_uid_l').val(), 16);
    const value = parseInt($('#led_value').val(), 16);

    const uid_h_ok = (uid_h >= 0) && (uid_h <= 255);
    const uid_l_ok = (uid_l >= 0) && (uid_l <= 255);
    const value_ok = (value == 0) || (value == 1);

    if (uid_h_ok && uid_l_ok && value_ok)
    {
        console.log("setled "
            + uid_h.toString(16).toUpperCase() + " "
            + uid_l.toString(16).toUpperCase() + " "
            + value.toString(16).toUpperCase()
        );

        send("setled "
            + uid_h.toString(16).toUpperCase() + " "
            + uid_l.toString(16).toUpperCase() + " "
            + value.toString(16).toUpperCase()
        );
    } else
    {
        alert("記入されたIDまたはValueの中に不適切なものがあります。");
        return;
    }

}

function setmode()
{
    if (!$('#mode_id').val())
    {
        alert("MODE IDが未記入です。");
        return;
    }

    const mode = parseInt($('#mode_id').val(), 16);

    if (mode >= 0 && mode <= 255)
    {
        console.log("setmode "
            + mode.toString(16).toUpperCase()
        );

        send("setmode "
            + mode.toString(16).toUpperCase()
        );
    } else
    {
        alert("MODE IDが不適切です。");
        return;
    }

}

function connect()
{
    if (webSocket == null)
    {
        console.log("WebSocket address = ws://" + location.hostname + "/ws .");
        webSocket = new WebSocket("ws://" + location.hostname + "/ws");
        webSocket.onopen = onOpen;
        webSocket.onmessage = onMessage;
        webSocket.onclose = onClose;
        webSocket.onerror = onError;
    }
}

function send(msg)
{
    if (webSocket != null)
    {
        webSocket.send("" + msg);
    }
}

function onOpen(event)
{
    alert("Connect");
}

function onMessage(event)
{
    if (event && event.data)
    {
        alert(event.data);
    }
}

function onClose(event)
{
    webSocket = null;
}

function onError(event) {}

