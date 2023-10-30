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
    //yb.addMessageId = true;

    setTimeout(yb.printMessageStats.bind(yb), 1000);    

    setTimeout(function () {
        if (options.host == "fullmain.local")
        {
            setTimeout(function (){togglePin(0, 5000)}, 1);

            // let foo = 50;
            // let bar = 10;
            // setTimeout(function (){fadePinHardware(0, foo+bar*0)}, 1);                
            // setTimeout(function (){fadePinHardware(1, foo+bar*1)}, 1);
            // setTimeout(function (){fadePinHardware(2, foo+bar*2)}, 1);
            // setTimeout(function (){fadePinHardware(3, foo+bar*3)}, 1);
            // setTimeout(function (){fadePinHardware(4, foo+bar*4)}, 1);
            // setTimeout(function (){fadePinHardware(5, foo+bar*5)}, 1);
            // setTimeout(function (){fadePinHardware(6, foo+bar*6)}, 1);
            // setTimeout(function (){fadePinHardware(7, foo+bar*7)}, 1);
        }
        else if (options.host == "firstreef.local")
        {
            setTimeout(function (){togglePin(0, 5)}, 1);
            // setTimeout(function (){togglePin(1, 100)}, 2);
            // setTimeout(function (){togglePin(2, 150)}, 3);
            // setTimeout(function (){togglePin(3, 200)}, 4);
            // setTimeout(function (){togglePin(4, 250)}, 5);
            // setTimeout(function (){togglePin(5, 300)}, 6);
            // setTimeout(function (){togglePin(6, 350)}, 7);
            // setTimeout(function (){togglePin(7, 400)}, 8);

            //cmd = {"cmd":"set_channel","id": 0, "state":true};
            //setTimeout(function (){speedTest(cmd, 10)}, 100);
        }
        else if (options.host == "8chrb.local")
        {
            let foo = 300;
            let bar = 10;
            // setTimeout(function (){fadePinHardware(0, foo+bar*0)}, 1);                
            // setTimeout(function (){fadePinHardware(1, foo+bar*1)}, 1);
            // setTimeout(function (){fadePinHardware(2, foo+bar*2)}, 1);
            // setTimeout(function (){fadePinHardware(3, foo+bar*3)}, 1);
            // setTimeout(function (){fadePinHardware(4, foo+bar*4)}, 1);
            // setTimeout(function (){fadePinHardware(5, foo+bar*5)}, 1);
            // setTimeout(function (){fadePinHardware(6, foo+bar*6)}, 1);
            //setTimeout(function (){fadePinHardware(7, foo+bar*7)}, 1);

            //setTimeout(function (){togglePin(7, 1000)}, 1);
            setTimeout(function (){fadePinManual(7, 25)}, 1);
        }
        else if (options.host == "rgbinput.local")
        {
            setTimeout(function (){rgbFadeManual(0, 25)}, 1);
        }
        else
        {
            setTimeout(function (){togglePin(0, 250)}, 1);
        }

        let cmd;
        cmd = {"cmd":"ping"}
        //cmd = {"cmd":"toggle_channel","id": 0};
        //cmd = {"cmd":"set_channel","id": 0, "state":true};
    
    }, 1000);

    yb.onmessage = function (msg) {
        if (msg.msgid)
            this.log(msg.msgid);
    }
    yb.start();
}

async function testFadeInterrupt()
{
    while (true)
    {
        yb.fadePWMChannel(2, 1, 1000);
        await delay(500);

        yb.fadePWMChannel(2, 0, 1000);
        await delay(500);
    }
}

async function testAllFade(d = 1000)
{
    while (true)
    {
        for (let i=0; i<8; i++)
            yb.fadePWMChannel(i, 1, d);

        await delay(d*2);

        for (let i=0; i<8; i++)
            yb.fadePWMChannel(i, 0, d);
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
            yb.setPWMChannelDuty(i, Math.random());
            yb.setPWMChannelState(i, true);

            await delay(200)
        }

        for (i=0; i<8; i++)
        {
            yb.setPWMChannelState(i, false);
           	await delay(200)
        }
    }
}

async function togglePin(channel = 0, d = 10)
{
    yb.setPWMChannelDuty(channel, 1);
    while (true)
    {
        yb.togglePWMChannel(channel);
        await delay(d)
    }
}

async function fadePinManual(channel = 0, d = 10)
{
    let steps = 25;
    let max_duty = 1;

    while (true)
    {
        yb.setPWMChannelState(channel, true);
        await delay(d)

        for (i=0; i<=steps; i++)
        {
            yb.setPWMChannelDuty(channel, (i / steps) * max_duty)
            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            yb.setPWMChannelDuty(channel, (i / steps) * max_duty)
            await delay(d)
        }

        await delay(d)
    }
}

async function rgbFadeManual(channel = 0, d = 25)
{
    let steps = 25;
    let max_duty = 1;

    while (true)
    {
        for (i=0; i<=steps; i++)
        {
            let duty = (i / steps) * max_duty;
            yb.setRGB(channel, duty, 0, 0);
            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            let duty = (i / steps) * max_duty;
            yb.setRGB(channel, duty, 0, 0);
            await delay(d)
        }

        for (i=0; i<=steps; i++)
        {
            let duty = (i / steps) * max_duty;
            yb.setRGB(channel, 0, duty, 0);
            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            let duty = (i / steps) * max_duty;
            yb.setRGB(channel, 0, duty, 0);
            await delay(d)
        }

        for (i=0; i<=steps; i++)
        {
            let duty = (i / steps) * max_duty;
            yb.setRGB(channel, 0, 0, duty);
            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            let duty = (i / steps) * max_duty;
            yb.setRGB(channel, 0, 0, duty);
            await delay(d)
        }

        await delay(d)
    }
}


async function fadePinHardware(channel = 0, d = 250, knee = 50)
{
    if (!knee)
        knee = d/2;

        while (true)
    {
        //yb.log(`fading to 1 in ${d}ms`);
        yb.fadePWMChannel(channel, 1, d);
        await delay(d+knee);

        //yb.log(`fading to 0 in ${d}ms`);
        yb.fadePWMChannel(channel, 0, d);
        await delay(d+knee);
    }
}

main();