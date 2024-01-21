import Phaser from "phaser";

const width = 725;
const height = 500;
const paddle = {
	width: 25,
	height: 100,
};

const color = 0xffffff;

class PongScene extends Phaser.Scene {
	private ball?: Phaser.GameObjects.Arc;
	private ballBody?: Phaser.Physics.Arcade.Body;
	private pL?: Phaser.GameObjects.Rectangle;
	private cursor?: any;

	create() {
		this.add.arc();
		this.ball = this.add.circle(width / 2, height / 2, 10, color);
		this.physics.add.existing(this.ball);
		this.ballBody = this.ball.body as Phaser.Physics.Arcade.Body;
		this.ballBody.setVelocity(400, 200 * Math.random());
		this.ballBody.setCollideWorldBounds(true);
		this.ballBody.setBounce(1, 1);

		this.pL = this.add.rectangle(
			100,
			height / 2,
			paddle.width,
			paddle.height,
			color,
		);
		this.physics.add.existing(this.pL, true);

		this.physics.add.collider(this.pL, this.ball);
		this.cursor = this.input.keyboard?.createCursorKeys()
	}
	update(): void {
		const {up, down} = this.cursor
		const player = this.pL!
		const body = player.body as Phaser.Physics.Arcade.StaticBody
		if(up.isDown && player.y > 60){
			player.y -= 10
			body.updateFromGameObject()
		}
		if(down.isDown && player.y < 435){
			console.log(player.y)
			player.y += 10
			body.updateFromGameObject()
		}
	}
}

new Phaser.Game({
	type: Phaser.AUTO,
	parent: "pong",
	width,
	height,
	backgroundColor: "#000000",
	scene: new PongScene(),
	physics: {
		default: "arcade",
		arcade: {
			gravity: {
				y: 0,
			},
		},
	},
});
