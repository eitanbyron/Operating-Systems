#include <linux/kernel.h>
#include<linux/sched.h>
#include <linux/list.h>
#include <linux/module.h>


asmlinkage long sys_hello(void) {
 printk("Hello, World!\n");
 return 0;
}

asmlinkage long sys_set_weight(int weight){
        if(weight < 0)
                return -EINVAL;
        current->weight = weight;
        return 0;
}


asmlinkage long sys_get_weight(void){
        return current->weight;
}

long calc_weight (struct task_struct* task_p){
	long total_weight=0;
	struct list_head* head;
	struct task_struct* child_task;
	if (list_empty (&task_p->children)){
		return task_p->weight;
	}
	
	list_for_each(head,&task_p->children){
		child_task=list_entry(head, struct task_struct, sibling);
		total_weight += calc_weight(child_task);
	}
	return total_weight;
}

asmlinkage long sys_get_leaf_children_sum(void){
	if (list_empty(&current -> children)){
		return -ECHILD;
	}
 
	return calc_weight(current);
}



asmlinkage long sys_get_heaviest_ancestor(void){
        struct task_struct* curr_task = current;
        long curr_weight = current->weight;
        pid_t max_pid = current->pid;

        while(curr_task->pid != 1){
                curr_task = curr_task->real_parent;

                if(curr_task->weight > curr_weight){
                        curr_weight = curr_task->weight;
                        max_pid = curr_task->pid;
                }
                
        }

        return max_pid;
}