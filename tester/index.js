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
        else if (options.host == "yarrboard.local")
        {
            setTimeout(function (){fadePinHardware(7, 2500)}, 1);
            // setTimeout(function (){fadePinHardware(6, 2000)}, 1);
            // setTimeout(function (){fadePinHardware(5, 1500)}, 1);
            // setTimeout(function (){fadePinHardware(4, 1000)}, 1);
            // setTimeout(function (){fadePinHardware(3, 500)}, 1);
            // setTimeout(function (){fadePinHardware(2, 300)}, 1);
            // setTimeout(function (){fadePinHardware(1, 250)}, 1);
            // setTimeout(function (){fadePinHardware(0, 100)}, 1);                
        }
//        setTimeout(function (){togglePin(0, 25)}, 1);
    
        //setTimeout(function (){fadePin(0, 8)}, 1);
    
    
        //setTimeout(testAllFade, 1);
    
        //setTimeout(testFadeInterrupt, 1);
    
        let cmd;
        cmd = {"cmd":"ping"}
        //cmd = {"cmd":"toggle_channel","id": 0};
        //cmd = {"cmd":"set_channel","id": 0, "state":true};
        //cmd = {"cmd":"set_channel","id": 0, "duty":0.5};
        //setTimeout(function (){speedTest(cmd, 10)}, 100);
    
    }
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
        yb.fadeChannel(2, 1, 1000);
        await delay(500);

        yb.fadeChannel(2, 0, 1000);
        await delay(500);
    }
}

async function testAllFade(d = 1000)
{
    while (true)
    {
        for (let i=0; i<8; i++)
            yb.fadeChannel(i, 1, d);

        await delay(d*2);

        for (let i=0; i<8; i++)
            yb.fadeChannel(i, 0, d);
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
            yb.setChannelDuty(i, Math.random());
            yb.setChannelState(i, true);

            await delay(200)
        }

        for (i=0; i<8; i++)
        {
            yb.setChannelState(i, false);
           	await delay(200)
        }
    }
}

async function togglePin(channel = 0, d = 10)
{
    yb.setChannelDuty(channel, 1);
    while (true)
    {
        yb.toggleChannel(channel);
        await delay(d)
    }
}

async function fadePinManual(channel = 0, d = 10)
{
    let steps = 25;
    let max_duty = 1;

    while (true)
    {
        yb.setChannelState(channel, true);
        await delay(d)

        for (i=0; i<=steps; i++)
        {
            yb.setChannelDuty(channel, (i / steps) * max_duty)
            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            yb.setChannelDuty(channel, (i / steps) * max_duty)
            await delay(d)
        }

        await delay(d)
    }
}

async function fadePinHardware(channel = 0, d = 250, knee = 0)
{
    if (!knee)
        knee = d/2;

        while (true)
    {
        //yb.log(`fading to 1 in ${d}ms`);
        yb.fadeChannel(channel, 1, d);
        await delay(d+knee);

        //yb.log(`fading to 0 in ${d}ms`);
        yb.fadeChannel(channel, 0, d);
        await delay(d+knee);
    }
}

main();