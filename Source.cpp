#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ShaderLoader.h"
#include "Camera.h"
#include "LightRenderer.h"
#include "TextureLoader.h"
#include "MeshRenderer.h"
#include "TextRenderer.h"
#include <iostream>
#include <chrono>

#include <btBulletDynamicsCommon.h>

/// <summary>
/// Testing how to use OpenGL for game development.
/// Created by following "C++ Game Development By Example" chapters 6-8 by Siddharth Shekar.
/// Changes include fixing many parts of the original code that were unimplemented in order to
/// allow it to build, changing textures, and adding a background.
/// </summary>

Camera* camera;
LightRenderer* light;
MeshRenderer* sphere;
MeshRenderer* ground;
MeshRenderer* bg;
MeshRenderer* enemy;
TextRenderer* label;
GLuint textProgram;
GLuint litTexturedShaderProgram;
bool grounded = false;
bool gameover = true;
int score = 0;

btDiscreteDynamicsWorld* dynamicsWorld;

//Prototypes
void renderScene();
void initGame();
void addRigidBodies(ShaderLoader shader);
void myTickCallback(btDynamicsWorld* dynamicsWorld, btScalar
	timeStep);
void checkCollision();

void updateKeyboard(GLFWwindow* window, int key, int scancode, int
	action, int mods) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		if (gameover) {
			gameover = false;
		}
		else {
			if (grounded == true) {
				grounded = false;
				sphere->rigidBody->applyImpulse(btVector3(0.0f, 100.0f, 0.0f),
					btVector3(0.0f, 0.0f, 0.0f));
				//printf("pressed up key \n");
			}
		}
	}
}

static void glfwError(int id, const char* description)
{
	std::cout << description << std::endl;
}

int main(int argc, char** argv)
{
	glfwInit();
	glfwSetErrorCallback(glfwError);
	GLFWwindow* window = glfwCreateWindow(800,
		600,
		" Hello OpenGL ",
		NULL,
		NULL);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, updateKeyboard);
	glewInit();
	initGame();
	auto previousTime = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(window)) {
		auto currentTime =
			std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float,
			std::chrono::seconds::period>(currentTime - previousTime).count();
		dynamicsWorld->stepSimulation(dt);
		// render our scene
		renderScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
		previousTime = currentTime;
	}
	glfwTerminate();
	delete camera;
	delete light;
	return 0;
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0, 1.0, 0.0, 1.0);//clear yellow
	// Draw game objects here
	//light->draw();
	sphere->draw();
	ground->draw();
	enemy->draw();
	bg->draw();

	label->draw();
}

void initGame() {
	glEnable(GL_DEPTH_TEST);
	

	//init physics
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();
	btDefaultCollisionConfiguration* collisionConfiguration = new
		btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new
		btCollisionDispatcher(collisionConfiguration);
	btSequentialImpulseConstraintSolver* solver = new
		btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase,
		solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, -9.8f, 0));
	dynamicsWorld->setInternalTickCallback(myTickCallback);

	ShaderLoader shader;
	litTexturedShaderProgram =
		shader.createProgram("Assets/Shaders/LitTexturedModel.vs",
			"Assets/Shaders/LitTexturedModel.fs");
	textProgram = shader.createProgram("Assets/Shaders/text.vs",
		"Assets/Shaders/text.fs");
	addRigidBodies(shader);

	
	label = new TextRenderer("Score: 0", "Assets/fonts/gooddog.ttf",
		64, glm::vec3(1.0f, 0.0f, 0.0f), textProgram);
	label->setPosition(glm::vec2(320.0f, 500.0f));
	

}

void addRigidBodies(ShaderLoader shader)
{
	
	GLuint flatShaderProgram =
		shader.createProgram("Assets/Shaders/FlatModel.vs",
			"Assets/Shaders/FlatModel.fs");
	GLuint texturedShaderProgram =
		shader.createProgram("Assets/Shaders/TexturedModel.vs",
			"Assets/Shaders/TexturedModel.fs");

	light = new LightRenderer(MeshType::kSphere, camera);
	light->setProgram(flatShaderProgram);
	light->setPosition(glm::vec3(0.0f, 10.0f, 0.0f));
	light->setColor(glm::vec3(1.0f, 1.0f, 1.0f));

	TextureLoader tLoader;
	GLuint sphereTexture = tLoader.getTextureID("Assets/Textures/tennisBall.jpg");
	GLuint groundTexture = tLoader.getTextureID("Assets/Textures/ground.jpg");
	GLuint bgTexture = tLoader.getTextureID("Assets/Textures/globe.jpg");
	GLuint enemyTexture = tLoader.getTextureID("Assets/Textures/lava.jpg");
	camera = new Camera(45.0f, 800, 600, 0.1f, 100.0f, glm::vec3(0.0f,
		4.0f, 20.0f));

	//Background rigid body
	btCollisionShape* bgShape = new btBoxShape(btVector3(30.0f, 30.0f,
		0.5f));
	btDefaultMotionState* bgMotionState = new
		btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
			btVector3(0, -5, -27)));
	btRigidBody::btRigidBodyConstructionInfo bgrbCI(0.0f, bgMotionState,
		bgShape, btVector3(0, 0, 0));
	btRigidBody* bgrb = new btRigidBody(bgrbCI);
	bgrb->setFriction(1.0);
	bgrb->setRestitution(0.0);
	//rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
	bgrb->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
	dynamicsWorld->addRigidBody(bgrb);

	//Background mesh
	bg = new MeshRenderer(MeshType::kCube, "bg", camera,
		bgrb, light, 0.1f, 0.5f);
	bg->setProgram(litTexturedShaderProgram);
	bg->setTexture(bgTexture);
	bg->setScale(glm::vec3(40, 40, 0.5f));

	// Sphere Rigid Body
	btCollisionShape* sphereShape = new btSphereShape(1);
	btDefaultMotionState* sphereMotionState = new
		btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
			btVector3(0, 0.5, 0)));
	btScalar mass = 13.0f;
	btVector3 sphereInertia(0, 0, 0);
	sphereShape->calculateLocalInertia(mass, sphereInertia);
	btRigidBody::btRigidBodyConstructionInfo sphereRigidBodyCI(mass,
		sphereMotionState, sphereShape, sphereInertia);
	btRigidBody* sphereRigidBody = new
		btRigidBody(sphereRigidBodyCI);
	sphereRigidBody->setFriction(1.0f);
	sphereRigidBody->setRestitution(0.0f);
	sphereRigidBody->setActivationState(DISABLE_DEACTIVATION);
	dynamicsWorld->addRigidBody(sphereRigidBody);
	// Sphere Mesh
	sphere = new MeshRenderer(MeshType::kSphere, "hero", camera,
		sphereRigidBody, light, 0.1f, 0.5f);
	sphere->setProgram(litTexturedShaderProgram);
	sphere->setTexture(sphereTexture);
	sphere->setScale(glm::vec3(1.0f));
	sphereRigidBody->setUserPointer(sphere);

	// Ground Rigid body
	btCollisionShape* groundShape = new btBoxShape(btVector3(4.0f,
		0.5f, 4.0f));
	btDefaultMotionState* groundMotionState = new
		btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
			btVector3(0, -1.0f, 0)));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0.0f,
		groundMotionState, groundShape, btVector3(0, 0, 0));
	btRigidBody* groundRigidBody = new
		btRigidBody(groundRigidBodyCI);
	groundRigidBody->setFriction(1.0);
	groundRigidBody->setRestitution(0.0);
	groundRigidBody->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
	dynamicsWorld->addRigidBody(groundRigidBody);
	// Ground Mesh
	ground = new MeshRenderer(MeshType::kCube, "ground", camera,
		groundRigidBody, light, 0.1f, 0.5f);
	ground->setProgram(litTexturedShaderProgram);
	ground->setTexture(groundTexture);
	ground->setScale(glm::vec3(4.0f, 0.5f, 4.0f));
	groundRigidBody->setUserPointer(ground);

	// Enemy Rigid body
	btCollisionShape* shape = new btBoxShape(btVector3(1.0f, 1.0f,
		1.0f));
	btDefaultMotionState* motionState = new
		btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
			btVector3(18.0f, 1.0f, 0)));
	btRigidBody::btRigidBodyConstructionInfo rbCI(0.0f, motionState,
		shape, btVector3(0, 0, 0));
	btRigidBody* rb = new btRigidBody(rbCI);
	rb->setFriction(1.0);
	rb->setRestitution(0.0);
	//rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
	rb->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
	dynamicsWorld->addRigidBody(rb);
	// Enemy Mesh
	enemy = new MeshRenderer(MeshType::kCube, "enemy", camera, rb,
		light, 1, 0.7f);
	enemy->setProgram(litTexturedShaderProgram);
	enemy->setTexture(enemyTexture);
	enemy->setScale(glm::vec3(1.0f, 1.0f, 1.0f));
	rb->setUserPointer(enemy);
}

void myTickCallback(btDynamicsWorld* dynamicsWorld, btScalar
	timeStep) {
	if (!gameover) {
		// Get enemy transform
		btTransform t(enemy->rigidBody->getWorldTransform());
		// Set enemy position
		t.setOrigin(t.getOrigin() + btVector3(-15, 0, 0) *
			timeStep);
		// Check if offScreen
		if (t.getOrigin().x() <= -18.0f) {
			t.setOrigin(btVector3(18, 1, 0));
			score++;
			label->setText("Score: " + std::to_string(score));
		}
		enemy->rigidBody->setWorldTransform(t);
		enemy->rigidBody->getMotionState()->setWorldTransform(t);
		grounded = false;
		checkCollision();
	}
}

void checkCollision()
{
	int numManifolds =
		dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		btPersistentManifold* contactManifold = dynamicsWorld -> getDispatcher()->getManifoldByIndexInternal(i);
		int numContacts = contactManifold->getNumContacts();
		if (numContacts > 0) {
			const btCollisionObject* objA = contactManifold -> getBody0();
			const btCollisionObject* objB = contactManifold -> getBody1();
			MeshRenderer* gModA = (MeshRenderer*)objA -> getUserPointer();
			MeshRenderer* gModB = (MeshRenderer*)objB -> getUserPointer();
			if ((gModA->name == "hero" && gModB->name ==
				"enemy") ||
				(gModA->name == "enemy" && gModB->name
					== "hero")) {
				//printf("collision: %s with %s \n", gModA->name, gModB->name);
				gameover = true;
				score = 0;
				label->setText("Score: " + std::to_string(score));
				if (gModB->name == "enemy") {
					btTransform b(gModB->rigidBody -> getWorldTransform());
					b.setOrigin(btVector3(18, 1, 0));
					gModB->rigidBody -> setWorldTransform(b);
					gModB->rigidBody->
						getMotionState() -> setWorldTransform(b);
				}
				else {
					btTransform a(gModA->rigidBody -> getWorldTransform());
					a.setOrigin(btVector3(18, 1, 0));
					gModA->rigidBody -> setWorldTransform(a);
					gModA->rigidBody->
						getMotionState()->
						setWorldTransform(a);
				}
			}
			if ((gModA->name == "hero" && gModB->name ==
				"ground") ||
				(gModA->name == "ground" && gModB->name
					== "hero")) {
				//printf("collision: %s with %s \n", gModA->name, gModB->name);
				grounded = true;
			}
		}
	}
}