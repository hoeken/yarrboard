#!/usr/bin/env node

const YarrboardClient = require('yarrboard-client');
const commander = require('commander');
const delay = (millis) => new Promise(resolve => setTimeout(resolve, millis)) 

commander
  .version('1.0.0', '-v, --version')
  .usage('[OPTIONS]...')
  .option('-h, --host <value>', 'Yarrboard hostname', 'yarrboard.local')
  .option('-u, --user <value>', 'Username', 'admin')
  .option('-p, --pass <value>', 'Password', 'admin')
  .option('-l, --login', 'Login or not')
  .parse(process.argv);

const options = commander.opts();

yb = new YarrboardClient(options.host, options.user, options.pass, options.login);

function main()
{
    yb = new YarrboardClient(options.host, options.user, options.pass, options.login);
    yb.onopen = function () {
        if (options.host == "fullmain.local")
        {
            setTimeout(function (){fadePinHardware(7, 2500)}, 1);
            setTimeout(function (){fadePinHardware(6, 2000)}, 1);
            setTimeout(function (){fadePinHardware(5, 1500)}, 1);
            setTimeout(function (){fadePinHardware(4, 1000)}, 1);
            setTimeout(function (){fadePinHardware(3, 500)}, 1);
            setTimeout(function (){fadePinHardware(2, 300)}, 1);
            setTimeout(function (){fadePinHardware(1, 250)}, 1);
            setTimeout(function (){fadePinHardware(0, 100)}, 1);                
        }
        else
            setTimeout(function (){togglePin(0, 25)}, 1);
    
        //setTimeout(function (){fadePin(0, 8)}, 1);
    
    
        //setTimeout(testAllFade, 1);
    
        //setTimeout(testFadeInterrupt, 1);
    
        let cmd;
        cmd = {"cmd":"ping"}
        //cmd = {"cmd":"toggle_channel","id": 0};
        //cmd = {"cmd":"set_channel","id": 0, "state":true};
        //cmd = {"cmd":"set_channel","id": 0, "duty":0.5};
        //setTimeout(function (){speedTest(cmd, 10)}, 100);
    
        setTimeout(yb.printMessageStats.bind(yb), 1000);    
    }
    yb.onmessage = function (msg) {
    }
    yb.start();
}

async function testFadeInterrupt()
{
    while (true)
    {
        yb.json({
            "cmd": "fade_channel",
            "id": 2,
            "duty": 1,
            "millis": 1000
        });

        await delay(500);

        yb.json({
            "cmd": "fade_channel",
            "id": 2,
            "duty": 0,
            "millis": 1000
        });

        await delay(500);
    }
}

async function testAllFade(d = 1000)
{
    while (true)
    {
        for (let i=0; i<8; i++)
            yb.json({
                "cmd": "fade_channel",
                "id": i,
                "duty": 1,
                "millis": d
            });

        await delay(d*2);

        for (let i=0; i<8; i++)
            yb.json({
                "cmd": "fade_channel",
                "id": i,
                "duty": 0,
                "millis": d
            });

            await delay(d*2);

    }
}

async function speedTest(msg, delay_ms = 10)
{
    while(true) {
        yb.json(msg);
        await delay(delay_ms);
    }
}

async function exercisePins()
{
    while (true) {
        for (i=0; i<8; i++)
        {
            yb.json({
                "cmd": "set_channel",
                "id": i,
                "duty": Math.random()
            });
            yb.json({
                "cmd": "set_channel",
                "id": i,
                "state": true
            });

            await delay(200)
        }

        for (i=0; i<8; i++)
        {
            yb.json({
                "cmd": "set_channel",
                "id": i,
                "state": false
            });
           	await delay(200)
        }
    }
}

async function togglePin()
{
    yb.json({
        "cmd": "set_channel",
        "id": 0,
        "duty": 1
    });

    while (true) {
        yb.json({
            "cmd": "set_channel",
            "id": 0,
            "state": true
        });

        await delay(1000)

        yb.json({
            "cmd": "set_channel",
            "id": 0,
            "state": false
        });

        await delay(2000)
    }
}

async function togglePin(channel = 0, d = 10)
{
    while (true)
    {
        yb.json({
            "cmd": "toggle_channel",
            "id": channel
        });
        await delay(d)
    }
}

async function fadePin(channel = 0, d = 10)
{
    let steps = 25;
    let max_duty = 1;

    while (true)
    {
        yb.json({
            "cmd": "set_channel",
            "id": channel,
            "state": true
        });
        await delay(d)

        for (i=0; i<=steps; i++)
        {
            yb.json({
                "cmd": "set_channel",
                "id": channel,
                "duty": (i / steps) * max_duty
            });

            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            yb.json({
                "cmd": "set_channel",
                "id": channel,
                "duty": (i / steps) * max_duty
            });

            await delay(d)
        }

        yb.json({
            "cmd": "set_channel",
            "id": channel,
            "state": true
        });
        await delay(d)
    }
}

async function fadePinHardware(channel = 0, d = 250)
{
    yb.json({
        "cmd": "set_channel",
        "id": channel,
        "state": true
    });
    await delay(10)

    while (true)
    {
        yb.json({
            "cmd": "fade_channel",
            "id": channel,
            "duty": 1,
            "millis": d
        });
        await delay(d+100);

        yb.json({
            "cmd": "fade_channel",
            "id": channel,
            "duty": 0,
            "millis": d
        });
        await delay(d+100);
    }
}

main();