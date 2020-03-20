using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BallSpawner : MonoBehaviour {

    public GameObject ball_prefab;
    public Color local_ball_color = Color.HSVToRGB(Random.value, 1.0f, 1.0f);
    public float spawn_bounds_x = 4.4f;
    public float spawn_bounds_y = 5.0f;
    public float spawn_bounds_z = 4.4f;
    public ExampleObservableManager manager;
    private Queue<GameObject> balls;

	// Use this for initialization
	void Start () {
        balls = new Queue<GameObject>();
    }
	
	// Update is called once per frame
	void Update () {
		
	}

    public void SpawnBall()
    {
        GameObject ball = GameObject.Instantiate(ball_prefab);
        ball.transform.SetParent(transform);
        ball.transform.position = new Vector3(Random.Range(-spawn_bounds_x, spawn_bounds_x), spawn_bounds_y, Random.Range(-spawn_bounds_z, spawn_bounds_z));
        balls.Enqueue(ball);
        //Create unity wrapper for ZST component
        TransformableComponent visual_component = ball.AddComponent<TransformableComponent>();
        visual_component.Init("ball" + transform.childCount, manager.client);
    }

    public void DestroyBall()
    {
        if(balls.Count > 0)
        {
            GameObject ball = balls.Dequeue();
            GameObject.Destroy(ball);
        }
    }
}
