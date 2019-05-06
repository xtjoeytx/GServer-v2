def notify(status){
	emailext (
		body: '$DEFAULT_CONTENT', 
		recipientProviders: [
			[$class: 'CulpritsRecipientProvider'],
			[$class: 'DevelopersRecipientProvider'],
			[$class: 'RequesterRecipientProvider']
		], 
		replyTo: '$DEFAULT_REPLYTO', 
		subject: '$DEFAULT_SUBJECT',
		to: '$DEFAULT_RECIPIENTS'
	)
}

@NonCPS
def killall_jobs() {
	def jobname = env.JOB_NAME
	def buildnum = env.BUILD_NUMBER.toInteger()
	def killnums = ""
	def job = Jenkins.instance.getItemByFullName(jobname)
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')

	for (build in job.builds) {
		if (!build.isBuilding()) { continue; }
		if (buildnum == build.getNumber().toInteger()) { continue; println "equals" }
		if (buildnum < build.getNumber().toInteger()) { continue; println "newer" }
		
		echo "Kill task = ${build}"
		
		killnums += "#" + build.getNumber().toInteger() + ", "
		
		build.doStop();
	}
	
	if (killnums != "") {
		slackSend color: "danger", channel: "#aros", message: "Killing task(s) ${fixed_job_name} ${killnums} in favor of #${buildnum}, ignore following failed builds for ${killnums}"
	}
	echo "Done killing"
}

def buildStep(dockerImage, generator, os, defines) {
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')
    def fixed_os = os.replace(' ','_')
	try{
		stage("Building on \"${dockerImage}\" with \"${generator}\" for \"${os}\"...") {
			properties([pipelineTriggers([githubPush()])])
			def commondir = env.WORKSPACE + '/../' + fixed_job_name + '/'

			checkout scm
			docker.image("${dockerImage}").withRun("-ti --name dockcross_${fixed_os} -v ${env.WORKSPACE}:/work -v /home/marlon/.ssh:/home/marlon/.ssh -e BUILDER_UID=1001 -e BUILDER_GID=1001 -e BUILDER_USER=marlon -e BUILDER_GROUP=marlon").inside  {
                if (env.CHANGE_ID) {
                    echo 'Trying to build pull request'
                    sh "echo \$HOSTNAME "
                }

                if (!env.CHANGE_ID) {
                    sh "rm -rfv ${env.WORKSPACE}/publishing/deploy/*"
                    sh "mkdir -p ${env.WORKSPACE}/publishing/deploy/gs2emu"
                }

			    slackSend color: "good", channel: "#jenkins", message: "Starting ${os} build target..."

	    		sh "cd ${env.WORKSPACE}/build && cmake -G\"${generator}\" .. ${defines} "

	    		sh "cd ${env.WORKSPACE}/build && cmake --build . --target package --config Release"
		
				slackSend color: "good", channel: "#jenkins", message: "Build ${fixed_job_name} #${env.BUILD_NUMBER} Target: ${os} DockerImage: ${dockerImage} Generator: ${generator} successful!"
			}
		}
	} catch(err) {
		slackSend color: "danger", channel: "#jenkins", message: "Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER} Target: ${os} DockerImage: ${dockerImage} Generator: ${generator} (<${env.BUILD_URL}|Open>)"
		currentBuild.result = 'FAILURE'
		notify('Build failed')
		throw err
	}
}

node('master') {
	killall_jobs()
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')
	slackSend color: "good", channel: "#jenkins", message: "Build Started: ${fixed_job_name} #${env.BUILD_NUMBER} (<${env.BUILD_URL}|Open>)"
	parallel (
		'Win64': {
			node {			
				buildStep('dockcross/windows-static-x64:latest', 'Unix Makefiles', 'Windows 64bit Static', "-DNOUPNP=TRUE -DNOSTATIC=FALSE -DV8NPCSERVER=FALSE")
			}
		},
		'Win32': {
			node {			
				buildStep('dockcross/windows-static-x86:latest', 'Unix Makefiles', 'Windows 32bit Static', "-DNOUPNP=TRUE -DNOSTATIC=FALSE -DV8NPCSERVER=FALSE")
			}
		}
    )
}